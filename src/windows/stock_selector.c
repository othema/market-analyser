#include "windows/stock_selector.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include "alphavantage/alphavantage.h"
#include "chart/chart.h"

#include <stdio.h>
#include <string.h>

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

// Fetches daily time-series data and company details for a ticker and updates chart state.
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
        av_time_series_free(&series);
        return;
    }
    av_time_series_free(&series);

    // Best-effort company details (overview). Logo endpoint may require premium access.
    AVCompanyOverview overview = {0};
    AVStatus overview_status = av_get_company_overview(&app->av.client, ticker, &overview);
    if (overview_status == AVOk) {
        chart_state_set_company_info(
            &app->chart,
            overview.name,
            overview.exchange,
            overview.currency,
            overview.sector,
            overview.industry,
            NULL
        );
    } else {
        chart_state_set_company_info(&app->chart, NULL, NULL, NULL, NULL, NULL, NULL);
    }
    av_company_overview_free(&overview);
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

    if (app && app->av.ready) {
        igTextDisabled("Alpha Vantage client ready");
    } else {
        igTextDisabled("Alpha Vantage client not initialized");
    }
}

