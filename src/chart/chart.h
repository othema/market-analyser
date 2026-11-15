#ifndef CHART_H
#define CHART_H

#include "yfinance/yf_types.h"

#include <stdbool.h>

// Holds chartable OHLC and company display data for the currently loaded ticker.
// Arrays are stored in chronological order for plotting.
typedef struct {
    char ticker[16];
    char company_name[128];
    char exchange[32];
    char currency[16];
    char sector[64];
    char industry[64];
    char logo_url[256];
    double latest_price;
    int has_latest_price;
    double *xs;
    double *opens;
    double *highs;
    double *lows;
    double *closes;
    long long *volumes;
    int count;
} ChartState;

// Initializes a ChartState to an empty, safe-to-clear state.
// Returns nothing.
void chart_state_init(ChartState *chart);

// Releases all heap memory owned by a ChartState and resets it to empty.
// Safe to call on an empty or already-cleared state. Returns nothing.
void chart_state_clear(ChartState *chart);

// Replaces chart contents with OHLC data copied from a parsed Yahoo Finance time series.
// Series points are expected in chronological order (oldest to newest).
// On failure, leaves the chart cleared.
// Returns true on success, or false on invalid input or allocation failure.
bool chart_state_set_from_yf_time_series(ChartState *chart, const char *ticker, const YFTimeSeries *series);

// Sets company display information on the chart state.
// Any NULL string argument leaves the corresponding field unchanged/cleared appropriately.
// Returns nothing.
void chart_state_set_company_info(ChartState *chart,
                                  const char *company_name,
                                  const char *exchange,
                                  const char *currency,
                                  const char *sector,
                                  const char *industry,
                                  const char *logo_url);

// Returns true if the chart has plottable OHLC data, otherwise false.
bool chart_state_has_data(const ChartState *chart);

#endif // CHART_H
