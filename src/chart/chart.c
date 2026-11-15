#include "chart/chart.h"

#include <stdlib.h>
#include <string.h>

// Copies a source string into a fixed destination buffer with NUL termination.
// Returns nothing.
static void chart_copy_string(char *dest, size_t dest_size, const char *src) {
    if (!dest || dest_size == 0) {
        return;
    }
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

// Initializes a ChartState to an empty, safe-to-clear state.
// Returns nothing.
void chart_state_init(ChartState *chart) {
    if (!chart) {
        return;
    }
    chart->ticker[0] = '\0';
    chart->company_name[0] = '\0';
    chart->exchange[0] = '\0';
    chart->currency[0] = '\0';
    chart->sector[0] = '\0';
    chart->industry[0] = '\0';
    chart->logo_url[0] = '\0';
    chart->latest_price = 0.0;
    chart->has_latest_price = 0;
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
    chart_copy_string(chart->ticker, sizeof(chart->ticker), ticker);
}

// Replaces chart contents with OHLC data copied from a parsed Yahoo Finance time series.
// Series points are expected in chronological order (oldest to newest).
// On failure, leaves the chart cleared.
// Returns true on success, or false on invalid input or allocation failure.
bool chart_state_set_from_yf_time_series(ChartState *chart, const char *ticker, const YFTimeSeries *series) {
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

    // Yahoo Finance series points are already in chronological order.
    for (size_t i = 0; i < count; i++) {
        const YFTimeSeriesPoint *point = &series->points[i];
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

    // Latest point is the last element in chronological order.
    chart->latest_price = series->points[count - 1].close;
    chart->has_latest_price = 1;

    return true;
}

// Sets company display information on the chart state.
// Any NULL string argument clears the corresponding field.
// Returns nothing.
void chart_state_set_company_info(ChartState *chart,
                                  const char *company_name,
                                  const char *exchange,
                                  const char *currency,
                                  const char *sector,
                                  const char *industry,
                                  const char *logo_url) {
    if (!chart) {
        return;
    }
    chart_copy_string(chart->company_name, sizeof(chart->company_name), company_name);
    chart_copy_string(chart->exchange, sizeof(chart->exchange), exchange);
    chart_copy_string(chart->currency, sizeof(chart->currency), currency);
    chart_copy_string(chart->sector, sizeof(chart->sector), sector);
    chart_copy_string(chart->industry, sizeof(chart->industry), industry);
    chart_copy_string(chart->logo_url, sizeof(chart->logo_url), logo_url);
}

// Returns true if the chart has plottable OHLC data, otherwise false.
bool chart_state_has_data(const ChartState *chart) {
    if (!chart) {
        return false;
    }
    return chart->count > 0 && chart->xs && chart->opens && chart->highs && chart->lows && chart->closes;
}
