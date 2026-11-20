#ifndef YF_TYPES_H
#define YF_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Represents a single data point in a Yahoo Finance chart time series.
typedef struct {
    char *date;                // Date string in YYYY-MM-DD format (UTC), heap-allocated.
    int64_t timestamp;         // Unix timestamp in seconds (UTC).
    double open;
    double high;
    double low;
    double close;
    double adjusted_close;     // Adjusted close if provided; 0.0 if unavailable.
    long long volume;
    int has_adjusted_close;
} YFTimeSeriesPoint;

// Represents a parsed Yahoo Finance chart time series result.
// Points are stored in chronological order (oldest to newest).
typedef struct {
    YFTimeSeriesPoint *points;
    size_t count;
    char *symbol;
    char *currency;
    char *exchange_name;
    char *instrument_type;
    char *long_name;
    char *short_name;
    char *sector;
    char *industry;
} YFTimeSeries;

// Represents a single symbol search suggestion.
typedef struct {
    char *symbol;
    char *short_name;
    char *long_name;
    char *exchange;
    char *quote_type;
} YFSearchResult;

// Frees all memory owned by an array of YFSearchResult items.
// Safe to call with NULL or count 0.
void yf_search_results_free(YFSearchResult *results, size_t count);

// Frees all memory owned by a YFTimeSeries and resets its fields.
// Safe to call on an empty or already-freed series.
void yf_time_series_free(YFTimeSeries *series);

#ifdef __cplusplus
}
#endif

#endif // YF_TYPES_H
