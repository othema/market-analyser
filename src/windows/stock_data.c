#include "windows/stock_data.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include "plot/plot_c.h"

/* Draws candlesticks (wick + open/close body) for the loaded OHLC data using ImPlot. */
static void draw_candlesticks(ChartState *chart) {
    if (!chart || !chart->xs || chart->ohlc_count <= 0) return;
    ImDrawList *dl = (ImDrawList *)plot_get_draw_list();
    if (!dl) return;
    for (int i = 0; i < chart->ohlc_count; i++) {
        double x = chart->xs[i];
        double open = chart->opens[i];
        double high = chart->highs[i];
        double low = chart->lows[i];
        double close = chart->closes[i];

        PlotVec2 pl = plot_to_pixels(x, low);
        PlotVec2 ph = plot_to_pixels(x, high);
        PlotVec2 po = plot_to_pixels(x, open);
        PlotVec2 pc = plot_to_pixels(x, close);
        ImVec2 p_low = {pl.x, pl.y};
        ImVec2 p_high = {ph.x, ph.y};
        ImVec2 p_open = {po.x, po.y};
        ImVec2 p_close = {pc.x, pc.y};

        ImU32 col = (close >= open) ? igGetColorU32_Vec4((ImVec4){0.20f, 0.80f, 0.20f, 1.0f})
                                    : igGetColorU32_Vec4((ImVec4){0.85f, 0.25f, 0.25f, 1.0f});

        ImDrawList_AddLine(dl, p_low, p_high, col, 1.0f);

        float half_w = 3.0f;
        float y_top = p_open.y < p_close.y ? p_open.y : p_close.y;
        float y_bot = p_open.y > p_close.y ? p_open.y : p_close.y;
        if (y_bot - y_top < 1.0f) y_bot = y_top + 1.0f;
        ImDrawList_AddRectFilled(dl, (ImVec2){p_open.x - half_w, y_top}, (ImVec2){p_open.x + half_w, y_bot}, col, 0.0f, 0);
    }
}

void stock_data_render(AppState *app) {
    igBegin("Stock Data", NULL, 0);

    if (!app || app->chart.ohlc_count <= 0) {
        igTextUnformatted("No data loaded. Enter a ticker and click \"View data\".", NULL);
        igEnd();
        return;
    }

    const char *label = (app->chart.ticker[0] != '\0') ? app->chart.ticker : "OHLC";
    PlotVec2 size = {-1.0f, 300.0f};
    if (plot_begin(label, size, 0)) {
        int n = app->chart.ohlc_count;
        double y_min = app->chart.lows[0];
        double y_max = app->chart.highs[0];
        for (int i = 1; i < n; i++) {
            if (app->chart.lows[i] < y_min) y_min = app->chart.lows[i];
            if (app->chart.highs[i] > y_max) y_max = app->chart.highs[i];
        }
        double pad = (y_max - y_min) * 0.05;
        if (pad <= 0.0) pad = 1.0;
        plot_setup_axes("Day", "Price", 0, 0);
        plot_setup_axes_limits(-1.0, (double)n, y_min - pad, y_max + pad, 1 /* ImPlotCond_Always */);
        draw_candlesticks(&app->chart);
        plot_end();
    }

    igEnd();
}
