#include "av_time_series.h"
#include "av_utils.h"

#include <stddef.h>

// Fetches a time-series function as CSV and parses it into out.
// Returns AVOk on success, or an AVErr* status on transport/API/parse failure.
static AVStatus av_get_time_series_common(AVClient *client, const char *function, const char *symbol, AVTimeSeries *out) {
    if (!client || !function || !symbol || !out) {
        return AVErrInvalidArg;
    }

    out->points = NULL;
    out->count = 0;

    AVParam params[] = {
        {"symbol", symbol},
        {"datatype", "csv"},
        {"outputsize", "compact"}
    };

    AVResponse response;
    av_response_init(&response);
    AVStatus status = av_query(client, function, params, 3, &response);
    if (status != AVOk) {
        av_response_free(&response);
        return status;
    }

    if (response.http_code != 200 || !response.data || av_response_is_error(response.data)) {
        av_response_free(&response);
        return AVErrCurl;
    }

    status = av_parse_time_series_csv(response.data, out);
    av_response_free(&response);
    return status;
}

// Fetches and parses a daily raw time series (TIME_SERIES_DAILY) for a symbol.
// Requests CSV (compact). On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_daily(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_DAILY", symbol, out);
}

// Fetches and parses a daily adjusted time series (TIME_SERIES_DAILY_ADJUSTED) for a symbol.
// Requests CSV (compact). On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_daily_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_DAILY_ADJUSTED", symbol, out);
}

// Fetches and parses a weekly raw time series (TIME_SERIES_WEEKLY) for a symbol.
// Requests CSV. On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_weekly(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_WEEKLY", symbol, out);
}

// Fetches and parses a weekly adjusted time series (TIME_SERIES_WEEKLY_ADJUSTED) for a symbol.
// Requests CSV. On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_weekly_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_WEEKLY_ADJUSTED", symbol, out);
}

// Fetches and parses a monthly raw time series (TIME_SERIES_MONTHLY) for a symbol.
// Requests CSV. On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_monthly(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_MONTHLY", symbol, out);
}

// Fetches and parses a monthly adjusted time series (TIME_SERIES_MONTHLY_ADJUSTED) for a symbol.
// Requests CSV. On success fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_time_series_monthly_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_MONTHLY_ADJUSTED", symbol, out);
}
