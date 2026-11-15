#include "windows/stock_data.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include "chart/chart.h"
#include "plot/plot_c.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Computes vertical axis bounds for chart data and writes them to output pointers.
// Returns nothing.
static void chart_get_price_bounds(const ChartState *chart, double *out_min, double *out_max) {
    if (!out_min || !out_max || !chart_state_has_data(chart)) {
        if (out_min) {
            *out_min = 0.0;
        }
        if (out_max) {
            *out_max = 1.0;
        }
        return;
    }

    double y_min = chart->lows[0];
    double y_max = chart->highs[0];
    for (int i = 1; i < chart->count; i++) {
        if (chart->lows[i] < y_min) {
            y_min = chart->lows[i];
        }
        if (chart->highs[i] > y_max) {
            y_max = chart->highs[i];
        }
    }
    *out_min = y_min;
    *out_max = y_max;
}

// Draws candlesticks (wick and open/close body) for the loaded OHLC data.
// Returns nothing.
static void draw_candlesticks(const ChartState *chart) {
    if (!chart_state_has_data(chart)) {
        return;
    }

    ImDrawList *draw_list = (ImDrawList *)plot_get_draw_list();
    if (!draw_list) {
        return;
    }

    for (int i = 0; i < chart->count; i++) {
        double x = chart->xs[i];
        double open = chart->opens[i];
        double high = chart->highs[i];
        double low = chart->lows[i];
        double close = chart->closes[i];

        PlotVec2 low_px = plot_to_pixels(x, low);
        PlotVec2 high_px = plot_to_pixels(x, high);
        PlotVec2 open_px = plot_to_pixels(x, open);
        PlotVec2 close_px = plot_to_pixels(x, close);

        ImVec2 p_low = {low_px.x, low_px.y};
        ImVec2 p_high = {high_px.x, high_px.y};
        ImVec2 p_open = {open_px.x, open_px.y};
        ImVec2 p_close = {close_px.x, close_px.y};

        ImU32 color = (close >= open)
            ? igGetColorU32_Vec4((ImVec4){0.20f, 0.80f, 0.20f, 1.0f})
            : igGetColorU32_Vec4((ImVec4){0.85f, 0.25f, 0.25f, 1.0f});

        ImDrawList_AddLine(draw_list, p_low, p_high, color, 1.0f);

        float half_width = 3.0f;
        float y_top = p_open.y < p_close.y ? p_open.y : p_close.y;
        float y_bottom = p_open.y > p_close.y ? p_open.y : p_close.y;
        if (y_bottom - y_top < 1.0f) {
            y_bottom = y_top + 1.0f;
        }
        ImDrawList_AddRectFilled(
            draw_list,
            (ImVec2){p_open.x - half_width, y_top},
            (ImVec2){p_open.x + half_width, y_bottom},
            color,
            0.0f,
            0
        );
    }
}

// Renders company details and the chart for the current chart state.
// Intended to be called inside an existing ImGui window/child region.
// Returns nothing.
void stock_data_render(AppState *app) {
    static int chart_type_index = 0; // 0 = Candlesticks, 1 = Line
    static int running_avg_index = 0; // 0=Disabled, 1=10, 2=20, 3=50
    const char *chart_type_options[] = {"Candlesticks", "Line"};
    const char *running_avg_options[] = {"Disabled", "10", "20", "50"};
    const int running_avg_period_values[] = {0, 10, 20, 50};
    bool loading = false;
    char loading_text[256] = {0};
    if (app) {
        pthread_mutex_lock(&app->fetch.mutex);
        loading = app->fetch.in_progress;
        if (loading) {
            snprintf(loading_text, sizeof(loading_text), "%s", app->fetch.status_text);
        }
        pthread_mutex_unlock(&app->fetch.mutex);
    }

    if (loading) {
        igTextUnformatted(loading_text[0] ? loading_text : "Loading stock data...", NULL);
        igProgressBar(-1.0f * (float)igGetTime(), (ImVec2){-1.0f, 0.0f}, NULL);
        igSpacing();
    }

    if (!app || !chart_state_has_data(&app->chart)) {
        if (!loading) {
            igTextUnformatted("No data loaded. Enter a ticker and click \"View data\".", NULL);
        }
        return;
    }

    const ChartState *chart = &app->chart;
    const char *display_name = chart->company_name[0] != '\0' ? chart->company_name : chart->ticker;

    // Company header details.
    if (igBeginChild_Str("CompanyHeader", (ImVec2){0.0f, 110.0f}, false, 0)) {
        if (igBeginChild_Str("CompanyDetails", (ImVec2){0.0f, 0.0f}, false, 0)) {
            igTextUnformatted(display_name, NULL);
            if (chart->has_latest_price) {
                if (chart->currency[0] != '\0') {
                    igText("Current price: %.2f %s", chart->latest_price, chart->currency);
                } else {
                    igText("Current price: %.2f", chart->latest_price);
                }
            } else {
                igTextUnformatted("Current price: N/A", NULL);
            }


            if (chart->exchange[0] != '\0') {
                igText("Exchange: %s", chart->exchange);
            }
            if (chart->sector[0] != '\0') {
                igText("Sector: %s", chart->sector);
            }
            if (chart->industry[0] != '\0') {
                igText("Industry: %s", chart->industry);
            }
            igText("Data points: %d", chart->count);
        }
        igEndChild();
    }
    igEndChild();

    igSeparator();

    igTextUnformatted("Chart type:", NULL);
    igSameLine(0.0f, 8.0f);
    igSetNextItemWidth(160.0f);
    igCombo_Str_arr("##ChartType", &chart_type_index, chart_type_options, 2, 2);
    igSameLine(0.0f, 16.0f);
    igTextUnformatted("Running average:", NULL);
    igSameLine(0.0f, 8.0f);
    igSetNextItemWidth(100.0f);
    igCombo_Str_arr("##RunningAvg", &running_avg_index, running_avg_options, 4, 4);

    const char *label = (chart->ticker[0] != '\0') ? chart->ticker : "OHLC";
    PlotVec2 size = {-1.0f, -1.0f};
    if (plot_begin(label, size, 0)) {
        double y_min = 0.0;
        double y_max = 1.0;
        chart_get_price_bounds(chart, &y_min, &y_max);

        double padding = (y_max - y_min) * 0.05;
        if (padding <= 0.0) {
            padding = 1.0;
        }

        plot_setup_axes("Date", "Price", 0, 0);
        // Use time scale on X axis and fit limits to loaded data.
        // ImAxis_X1 = 0, ImPlotScale_Time = 1 in ImPlot v0.17.
        plot_setup_axis_scale(0, 1);

        // Set custom date tick labels: DD/MM, and DD/MM/YYYY when year changes.
        enum { MAX_TICKS = 8 };
        double tick_values[MAX_TICKS];
        char tick_labels[MAX_TICKS][32];
        const char *tick_label_ptrs[MAX_TICKS];
        int tick_count = chart->count < MAX_TICKS ? chart->count : MAX_TICKS;
        if (tick_count < 2) {
            tick_count = chart->count;
        }

        int prev_year = -10000;
        for (int t = 0; t < tick_count; t++) {
            int idx = (tick_count == 1) ? 0 : (t * (chart->count - 1)) / (tick_count - 1);
            if (idx < 0) idx = 0;
            if (idx >= chart->count) idx = chart->count - 1;

            tick_values[t] = chart->xs[idx];
            time_t ts = (time_t)chart->xs[idx];
            struct tm tm_buf;
            struct tm *tm_info = gmtime(&ts);
            if (tm_info) {
                tm_buf = *tm_info;
                tm_info = &tm_buf;
            }
            if (tm_info) {
                int year = tm_info->tm_year + 1900;
                if (t == 0 || year != prev_year) {
                    snprintf(tick_labels[t], sizeof(tick_labels[t]), "%02d/%02d/%04d",
                             tm_info->tm_mday, tm_info->tm_mon + 1, year);
                } else {
                    snprintf(tick_labels[t], sizeof(tick_labels[t]), "%02d/%02d",
                             tm_info->tm_mday, tm_info->tm_mon + 1);
                }
                prev_year = year;
            } else {
                snprintf(tick_labels[t], sizeof(tick_labels[t]), "?");
            }
            tick_label_ptrs[t] = tick_labels[t];
        }

        if (tick_count > 0) {
            // ImAxis_X1 = 0
            plot_setup_axis_ticks(0, tick_values, tick_count, tick_label_ptrs);
        }

        double x_min = chart->xs[0];
        double x_max = chart->xs[chart->count - 1];
        double x_pad = (x_max - x_min) * 0.02;
        if (x_pad <= 0.0) {
            x_pad = 86400.0;
        }
        // Use ImPlotCond_Always (1) so limits update when new data is loaded.
        plot_setup_axes_limits(x_min - x_pad, x_max + x_pad, y_min - padding, y_max + padding, 1);
        if (chart_type_index == 1) {
            plot_plot_line("Close", chart->xs, chart->closes, chart->count, 0);
        } else {
            draw_candlesticks(chart);
        }

        int running_avg_period = running_avg_period_values[running_avg_index];
        if (running_avg_period > 0 && chart->count > 0) {
            double *avg = (double *)malloc((size_t)chart->count * sizeof(double));
            if (avg) {
                double window_sum = 0.0;
                for (int i = 0; i < chart->count; i++) {
                    window_sum += chart->closes[i];
                    if (i >= running_avg_period) {
                        window_sum -= chart->closes[i - running_avg_period];
                    }
                    int denom = (i + 1 < running_avg_period) ? (i + 1) : running_avg_period;
                    avg[i] = window_sum / (double)denom;
                }
                char avg_label[32];
                snprintf(avg_label, sizeof(avg_label), "SMA %d", running_avg_period);
                // Use a consistent color for running average line.
                plot_set_next_line_style(1.00f, 0.70f, 0.20f, 1.00f, 2.0f);
                plot_plot_line(avg_label, chart->xs, avg, chart->count, 0);
                free(avg);
            }
        }

        // Interactive hover: vertical crosshair + tooltip for nearest point.
        if (plot_is_hovered() && chart->count > 0) {
            PlotPointD mouse = plot_get_mouse_pos(0, 3); // ImAxis_X1, ImAxis_Y1
            int nearest = 0;
            double best_dist = mouse.x - chart->xs[0];
            if (best_dist < 0.0) best_dist = -best_dist;
            for (int i = 1; i < chart->count; i++) {
                double d = mouse.x - chart->xs[i];
                if (d < 0.0) d = -d;
                if (d < best_dist) {
                    best_dist = d;
                    nearest = i;
                }
            }

            PlotRectD limits = plot_get_plot_limits(0, 3);
            ImDrawList *draw_list = (ImDrawList *)plot_get_draw_list();
            if (draw_list) {
                double x = chart->xs[nearest];
                PlotVec2 top = plot_to_pixels(x, limits.y_max);
                PlotVec2 bot = plot_to_pixels(x, limits.y_min);
                ImU32 cross_color = igGetColorU32_Vec4((ImVec4){0.80f, 0.80f, 0.80f, 0.45f});
                ImDrawList_AddLine(draw_list, (ImVec2){top.x, top.y}, (ImVec2){bot.x, bot.y}, cross_color, 1.0f);
            }

            // Format date label for tooltip.
            char date_label[32] = "?";
            time_t ts = (time_t)chart->xs[nearest];
            struct tm tm_buf;
            struct tm *tm_info = gmtime(&ts);
            if (tm_info) {
                tm_buf = *tm_info;
                snprintf(date_label, sizeof(date_label), "%02d/%02d/%04d",
                         tm_buf.tm_mday, tm_buf.tm_mon + 1, tm_buf.tm_year + 1900);
            }

            double open = chart->opens[nearest];
            double high = chart->highs[nearest];
            double low = chart->lows[nearest];
            double close = chart->closes[nearest];
            long long volume = chart->volumes ? chart->volumes[nearest] : 0;
            double change = 0.0;
            double change_pct = 0.0;
            if (nearest > 0) {
                double prev_close = chart->closes[nearest - 1];
                change = close - prev_close;
                if (prev_close != 0.0) {
                    change_pct = (change / prev_close) * 100.0;
                }
            }

            igBeginTooltip();
            igText("%s", date_label);
            igSeparator();
            igText("Open:   %.2f", open);
            igText("High:   %.2f", high);
            igText("Low:    %.2f", low);
            igText("Close:  %.2f", close);
            igText("Volume: %lld", volume);
            if (nearest > 0) {
                igText("Change: %+.2f (%+.2f%%)", change, change_pct);
            } else {
                igTextUnformatted("Change: N/A", NULL);
            }
            igEndTooltip();
        }

        plot_end();
    }
}

