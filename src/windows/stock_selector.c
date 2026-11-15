#include "windows/stock_selector.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include "chart/chart.h"
#include "yfinance/yfinance.h"

#include <stdio.h>
#include <string.h>

// Prints a parsed Yahoo Finance time series to stdout for quick inspection/debugging.
// Returns nothing.
static void stock_selector_print_series(const char *ticker, const YFTimeSeries *series) {
    if (!ticker || !series) {
        return;
    }

    printf("Stock data for %s (%zu points):\n", ticker, series->count);
    printf("%-12s %10s %10s %10s %10s %12s\n", "date", "open", "high", "low", "close", "volume");
    for (size_t i = 0; i < series->count; i++) {
        const YFTimeSeriesPoint *point = &series->points[i];
        printf("%-12s %10.2f %10.2f %10.2f %10.2f %12lld\n",
               point->date ? point->date : "?",
               point->open,
               point->high,
               point->low,
               point->close,
               point->volume);
    }
}

// Fetches daily time-series data for a ticker and updates chart state.
// Returns nothing.
static void stock_selector_fetch_and_update_chart(AppState *app, const char *ticker) {
    if (!app || !ticker || ticker[0] == '\0') {
        printf("No ticker entered.\n");
        fflush(stdout);
        return;
    }
    if (!app->yf.ready) {
        printf("Yahoo Finance client is not initialized.\n");
        fflush(stdout);
        return;
    }

    YFTimeSeries series = {0};
    YFStatus status = yf_get_time_series(&app->yf.client, ticker, "1d", "6mo", &series);
    if (status != YFOk) {
        printf("Failed to fetch stock data for %s: %s\n", ticker, yf_status_str(status));
        fflush(stdout);
        yf_time_series_free(&series);
        return;
    }

    stock_selector_print_series(ticker, &series);

    if (!chart_state_set_from_yf_time_series(&app->chart, ticker, &series)) {
        printf("Failed to prepare chart data for %s.\n", ticker);
        fflush(stdout);
        yf_time_series_free(&series);
        return;
    }

    // Yahoo Finance chart metadata provides limited company info.
    const char *company_name = series.symbol ? series.symbol : ticker;
    chart_state_set_company_info(
        &app->chart,
        company_name,
        series.exchange_name,
        series.currency,
        NULL,
        NULL,
        NULL
    );

    yf_time_series_free(&series);
    fflush(stdout);
}

// Renders stock selector controls and handles fetch requests from the UI.
// Intended to be called inside an existing ImGui window/child region.
// Returns nothing.
void stock_selector_render(AppState *app, char *ticker, int ticker_size) {
    igTextUnformatted("Symbol", NULL);
    igInputTextWithHint("##TickerInput", "AAPL", ticker, ticker_size, ImGuiInputTextFlags_None, NULL, NULL);
    igSameLine(0.0f, -1.0f);
    if (igButton("View data", (ImVec2){0, 0})) {
        stock_selector_fetch_and_update_chart(app, ticker);
        fflush(stdout);
    }

    if (app && app->yf.ready) {
        igTextDisabled("Yahoo Finance client ready");
    } else {
        igTextDisabled("Yahoo Finance client not initialized");
    }
}

