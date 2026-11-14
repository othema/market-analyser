#ifndef AV_TIME_SERIES_H
#define AV_TIME_SERIES_H

#include "alphavantage.h"
#include "av_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parses a time series from a raw Alpha Vantage CSV response body.
 * Supports daily/weekly/monthly series in both raw and adjusted forms by
 * detecting columns from the header row (e.g. timestamp/date, open, high, low,
 * close, adjusted_close, volume, dividend_amount, split_coefficient).
 * On success, allocates and fills *out (caller must free with av_time_series_free).
 * Returns AVOk on success, AVErrInvalidArg for bad inputs, AVErrNomem on
 * allocation failure, or AVErrBuffer/AVErrCurl if the content is not a valid
 * parseable time series CSV (e.g. an API error payload). */
AVStatus av_parse_time_series_csv(const char *csv_data, AVTimeSeries *out);

/* Fetches and parses a daily raw time series (TIME_SERIES_DAILY) for a symbol.
 * Requests CSV from the API (outputsize defaults to "compact" unless provided via
 * extra params mechanism is not exposed; this helper uses compact). On success,
 * fills *out (caller must free with av_time_series_free).
 * Returns AVOk on success, or an AVErr* / parse error status on failure. */
AVStatus av_get_time_series_daily(AVClient *client, const char *symbol, AVTimeSeries *out);

/* Fetches and parses a daily adjusted time series (TIME_SERIES_DAILY_ADJUSTED).
 * Requests CSV (compact). On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_daily_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out);

/* Fetches and parses a weekly raw time series (TIME_SERIES_WEEKLY).
 * Requests CSV. On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_weekly(AVClient *client, const char *symbol, AVTimeSeries *out);

/* Fetches and parses a weekly adjusted time series (TIME_SERIES_WEEKLY_ADJUSTED).
 * Requests CSV. On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_weekly_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out);

/* Fetches and parses a monthly raw time series (TIME_SERIES_MONTHLY).
 * Requests CSV. On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_monthly(AVClient *client, const char *symbol, AVTimeSeries *out);

/* Fetches and parses a monthly adjusted time series (TIME_SERIES_MONTHLY_ADJUSTED).
 * Requests CSV. On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_monthly_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out);

#ifdef __cplusplus
}
#endif

#endif /* AV_TIME_SERIES_H */
