#include "windows/stock_selector.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include "alphavantage/alphavantage.h"
#include "chart/chart.h"

#include <stdio.h>

// Prints a parsed time series to stdout for quick inspection/debugging.
// Returns nothing.
static void stock_selector_print_series(const char *ticker, const AVTimeSeries *series) {
    if (!ticker || !series) {
        return;
    }

    printf("Stock data for %s (%zu points):\n", ticker, series->count);
    printf("%-12s %10s %10s %10s %10s %12s\n", "date", "open", "high", "low", "close", "volume");
    for (size_t i = 0; i < series->count; i++) {
        const AVTimeSeriesPoint *point = &series->points[i];
        printf("%-12s %10.2f %10.2f %10.2f %10.2f %12lld\n",
               point->date ? point->date : "?",
               point->open,
               point->high,
               point->low,
               point->close,
               (long long)point->volume);
    }
}

// Fetches daily time-series data for a ticker and updates application chart state.
// Returns nothing.
static void stock_selector_fetch_and_update_chart(AppState *app, const char *ticker) {
    if (!app || !ticker || ticker[0] == '\0') {
        printf("No ticker entered.\n");
        return;
    }
    if (!app->av.ready) {
        printf("Alpha Vantage client is not initialized.\n");
        return;
    }

    AVTimeSeries series = {0};
    AVStatus status = av_get_time_series_daily(&app->av.client, ticker, &series);
    if (status != AVOk) {
        printf("Failed to fetch stock data for %s: %s\n", ticker, av_status_str(status));
        av_time_series_free(&series);
        return;
    }

    stock_selector_print_series(ticker, &series);
    if (!chart_state_set_from_time_series(&app->chart, ticker, &series)) {
        printf("Failed to prepare chart data for %s.\n", ticker);
    }

    av_time_series_free(&series);
}

// Renders the stock selector window and handles fetch requests from the UI.
// Returns nothing.
void stock_selector_render(AppState *app, char *ticker, int ticker_size) {
    igBegin("Stock Selector", NULL, 0);

    igInputTextWithHint("Ticker", "AAPL", ticker, ticker_size, ImGuiInputTextFlags_None, NULL, NULL);
    if (igButton("View data", (ImVec2){0, 0})) {
        stock_selector_fetch_and_update_chart(app, ticker);
        fflush(stdout);
    }

    igEnd();
}

