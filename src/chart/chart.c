#include "chart/chart.h"

#include <stdlib.h>
#include <string.h>

// Initializes a ChartState to an empty, safe-to-clear state.
// Returns nothing.
void chart_state_init(ChartState *chart) {
    if (!chart) {
        return;
    }
    chart->ticker[0] = '\0';
    chart->xs = NULL;
    chart->opens = NULL;
    chart->highs = NULL;
    chart->lows = NULL;
    chart->closes = NULL;
    chart->count = 0;
}

// Releases all heap memory owned by a ChartState and resets it to empty.
// Safe to call on an empty or already-cleared state. Returns nothing.
void chart_state_clear(ChartState *chart) {
    if (!chart) {
        return;
    }
    free(chart->xs);
    free(chart->opens);
    free(chart->highs);
    free(chart->lows);
    free(chart->closes);
    chart_state_init(chart);
}

// Copies a ticker string into chart state with bounds checking and NUL termination.
// Returns nothing.
static void chart_state_set_ticker(ChartState *chart, const char *ticker) {
    if (!chart) {
        return;
    }
    if (!ticker) {
        chart->ticker[0] = '\0';
        return;
    }
    strncpy(chart->ticker, ticker, sizeof(chart->ticker) - 1);
    chart->ticker[sizeof(chart->ticker) - 1] = '\0';
}

// Replaces chart contents with OHLC data copied from a parsed time series.
// Series points are expected in Alpha Vantage order (most recent first) and are
// stored chronologically. On failure, leaves the chart cleared.
// Returns true on success, or false on invalid input or allocation failure.
bool chart_state_set_from_time_series(ChartState *chart, const char *ticker, const AVTimeSeries *series) {
    if (!chart) {
        return false;
    }

    chart_state_clear(chart);

    if (!series || series->count == 0 || !series->points) {
        chart_state_set_ticker(chart, ticker);
        return false;
    }

    size_t count = series->count;
    double *xs = (double *)malloc(count * sizeof(double));
    double *opens = (double *)malloc(count * sizeof(double));
    double *highs = (double *)malloc(count * sizeof(double));
    double *lows = (double *)malloc(count * sizeof(double));
    double *closes = (double *)malloc(count * sizeof(double));
    if (!xs || !opens || !highs || !lows || !closes) {
        free(xs);
        free(opens);
        free(highs);
        free(lows);
        free(closes);
        return false;
    }

    // API returns most recent first; reverse to chronological for plotting.
    for (size_t i = 0; i < count; i++) {
        size_t src = count - 1 - i;
        const AVTimeSeriesPoint *point = &series->points[src];
        xs[i] = (double)i;
        opens[i] = point->open;
        highs[i] = point->high;
        lows[i] = point->low;
        closes[i] = point->close;
    }

    chart->xs = xs;
    chart->opens = opens;
    chart->highs = highs;
    chart->lows = lows;
    chart->closes = closes;
    chart->count = (int)count;
    chart_state_set_ticker(chart, ticker);
    return true;
}

// Returns true if the chart has plottable OHLC data, otherwise false.
bool chart_state_has_data(const ChartState *chart) {
    if (!chart) {
        return false;
    }
    return chart->count > 0 && chart->xs && chart->opens && chart->highs && chart->lows && chart->closes;
}
