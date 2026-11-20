#ifndef YF_CHART_H
#define YF_CHART_H

#include "yfinance.h"
#include "yf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parses Yahoo Finance chart JSON into a YFTimeSeries.
// On success, fills *out (caller must free with yf_time_series_free).
YFStatus yf_parse_chart_json(const char *json_data, YFTimeSeries *out);

// Fetches and parses a chart time series for a symbol.
// interval examples: "1d", "1wk", "1mo". range examples: "1mo", "6mo", "1y", "5y", "max".
// On success, fills *out (caller must free with yf_time_series_free).
YFStatus yf_get_time_series(YFClient *client,
                            const char *symbol,
                            const char *interval,
                            const char *range,
                            YFTimeSeries *out);

// Fetches and applies additional company information (name, sector, industry, exchange)
// from Yahoo Finance search results into an existing series.
// Best-effort: returns YFOk even if enrichment data is unavailable, but fills fields when found.
YFStatus yf_enrich_company_info(YFClient *client, const char *symbol, YFTimeSeries *series);

// Searches Yahoo Finance symbols matching a query string.
// On success, allocates *out_results (array of count items) which must be freed with yf_search_results_free.
YFStatus yf_search_symbols(YFClient *client, const char *query, YFSearchResult **out_results, size_t *out_count, size_t max_results);

#ifdef __cplusplus
}
#endif

#endif // YF_CHART_H
