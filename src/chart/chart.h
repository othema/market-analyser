#ifndef CHART_H
#define CHART_H

#include "alphavantage/av_types.h"

#include <stdbool.h>

// Holds chartable OHLC data for the currently loaded ticker.
// Arrays are stored in chronological order for plotting.
typedef struct {
    char ticker[16];
    double *xs;
    double *opens;
    double *highs;
    double *lows;
    double *closes;
    int count;
} ChartState;

// Initializes a ChartState to an empty, safe-to-clear state.
// Returns nothing.
void chart_state_init(ChartState *chart);

// Releases all heap memory owned by a ChartState and resets it to empty.
// Safe to call on an empty or already-cleared state. Returns nothing.
void chart_state_clear(ChartState *chart);

// Replaces chart contents with OHLC data copied from a parsed time series.
// Series points are expected in Alpha Vantage order (most recent first) and are
// stored chronologically. On failure, leaves the chart cleared.
// Returns true on success, or false on invalid input or allocation failure.
bool chart_state_set_from_time_series(ChartState *chart, const char *ticker, const AVTimeSeries *series);

// Returns true if the chart has plottable OHLC data, otherwise false.
bool chart_state_has_data(const ChartState *chart);

#endif // CHART_H
