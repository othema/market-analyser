#ifndef YF_TYPES_H
#define YF_TYPES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Represents a single data point in a Yahoo Finance chart time series.
typedef struct {
    char *date;                // Date string in YYYY-MM-DD format (UTC), heap-allocated.
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
} YFTimeSeries;

// Frees all memory owned by a YFTimeSeries and resets its fields.
// Safe to call on an empty or already-freed series. Returns nothing.
void yf_time_series_free(YFTimeSeries *series);

#ifdef __cplusplus
}
#endif

#endif // YF_TYPES_H
