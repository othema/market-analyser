#ifndef YF_CHART_H
#define YF_CHART_H

#include "yfinance.h"
#include "yf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parses Yahoo Finance chart JSON into a YFTimeSeries.
// On success, fills *out (caller must free with yf_time_series_free).
// Returns YFOk on success, or a YFErr* status on failure.
YFStatus yf_parse_chart_json(const char *json_data, YFTimeSeries *out);

// Fetches and parses a chart time series for a symbol.
// interval examples: "1d", "1wk", "1mo". range examples: "1mo", "6mo", "1y", "5y", "max".
// On success, fills *out (caller must free with yf_time_series_free).
// Returns YFOk on success, or a YFErr* status on failure.
YFStatus yf_get_time_series(YFClient *client,
                            const char *symbol,
                            const char *interval,
                            const char *range,
                            YFTimeSeries *out);

#ifdef __cplusplus
}
#endif

#endif // YF_CHART_H
