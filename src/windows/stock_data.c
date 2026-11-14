#include "windows/stock_data.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include "chart/chart.h"
#include "plot/plot_c.h"

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

// Renders stock data and the candlestick chart for the current chart state.
// Intended to be called inside an existing ImGui window/child region.
// Returns nothing.
void stock_data_render(AppState *app) {
    igTextUnformatted("Chart", NULL);
    igSeparator();

    if (!app || !chart_state_has_data(&app->chart)) {
        igTextUnformatted("No data loaded. Enter a ticker and click \"View data\".", NULL);
        return;
    }

    igText("Ticker: %s", app->chart.ticker[0] != '\0' ? app->chart.ticker : "N/A");
    igText("Points: %d", app->chart.count);

    const char *label = (app->chart.ticker[0] != '\0') ? app->chart.ticker : "OHLC";
    PlotVec2 size = {-1.0f, -1.0f};
    if (plot_begin(label, size, 0)) {
        double y_min = 0.0;
        double y_max = 1.0;
        chart_get_price_bounds(&app->chart, &y_min, &y_max);

        double padding = (y_max - y_min) * 0.05;
        if (padding <= 0.0) {
            padding = 1.0;
        }

        plot_setup_axes("Day", "Price", 0, 0);
        // Use ImPlotCond_Always (1) so limits update when new data is loaded.
        plot_setup_axes_limits(-1.0, (double)app->chart.count, y_min - padding, y_max + padding, 1);
        draw_candlesticks(&app->chart);
        plot_end();
    }
}

