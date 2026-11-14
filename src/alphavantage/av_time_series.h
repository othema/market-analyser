#ifndef AV_TIME_SERIES_H
#define AV_TIME_SERIES_H

#include "alphavantage.h"
#include "av_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parses a time series from a raw Alpha Vantage CSV response body.
// Supports daily/weekly/monthly series in raw and adjusted forms by detecting columns from the header.
// On success, fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_parse_time_series_csv(const char *csv_data, AVTimeSeries *out);

// Fetches and parses a daily raw time series (TIME_SERIES_DAILY) for a symbol.
// Requests CSV (compact). On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_daily(AVClient *client, const char *symbol, AVTimeSeries *out);

// Fetches and parses a daily adjusted time series (TIME_SERIES_DAILY_ADJUSTED) for a symbol.
// Requests CSV (compact). On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_daily_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out);

// Fetches and parses a weekly raw time series (TIME_SERIES_WEEKLY) for a symbol.
// Requests CSV. On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_weekly(AVClient *client, const char *symbol, AVTimeSeries *out);

// Fetches and parses a weekly adjusted time series (TIME_SERIES_WEEKLY_ADJUSTED) for a symbol.
// Requests CSV. On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_weekly_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out);

// Fetches and parses a monthly raw time series (TIME_SERIES_MONTHLY) for a symbol.
// Requests CSV. On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_monthly(AVClient *client, const char *symbol, AVTimeSeries *out);

// Fetches and parses a monthly adjusted time series (TIME_SERIES_MONTHLY_ADJUSTED) for a symbol.
// Requests CSV. On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_monthly_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out);

#ifdef __cplusplus
}
#endif

#endif // AV_TIME_SERIES_H
