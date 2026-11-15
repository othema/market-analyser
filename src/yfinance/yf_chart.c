#include "yf_chart.h"

#include <json-c/json.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Duplicates a NUL-terminated string onto the heap.
// Returns a newly allocated copy on success, or NULL if s is NULL or allocation fails.
static char *yf_strdup_safe(const char *s) {
    if (!s) {
        return NULL;
    }

    size_t length = strlen(s);
    char *copy = (char *)malloc(length + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, s, length + 1);
    return copy;
}

// Frees all memory owned by a YFTimeSeries and resets its fields.
// Safe to call on an empty or already-freed series. Returns nothing.
void yf_time_series_free(YFTimeSeries *series) {
    if (!series) {
        return;
    }

    for (size_t i = 0; i < series->count; i++) {
        free(series->points[i].date);
        series->points[i].date = NULL;
    }

    free(series->points);
    free(series->symbol);
    free(series->currency);
    free(series->exchange_name);
    free(series->instrument_type);

    series->points = NULL;
    series->count = 0;
    series->symbol = NULL;
    series->currency = NULL;
    series->exchange_name = NULL;
    series->instrument_type = NULL;
}

// Formats a Unix timestamp (seconds, UTC) as YYYY-MM-DD.
// Returns a heap-allocated string on success, or NULL on failure.
static char *yf_format_unix_date(int64_t timestamp_seconds) {
    time_t raw_time = (time_t)timestamp_seconds;
    struct tm *utc_time = gmtime(&raw_time);
    if (!utc_time) {
        return NULL;
    }

    char *buffer = (char *)malloc(11);
    if (!buffer) {
        return NULL;
    }

    if (strftime(buffer, 11, "%Y-%m-%d", utc_time) == 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

// Reads a number value from a json-c array element as double.
// Returns 1 on success and stores value in *out, or 0 if null/missing/invalid.
static int yf_json_array_get_double(struct json_object *array_obj, size_t index, double *out) {
    if (out) {
        *out = 0.0;
    }
    if (!array_obj || !out || !json_object_is_type(array_obj, json_type_array)) {
        return 0;
    }

    struct json_object *value = json_object_array_get_idx(array_obj, (int)index);
    if (!value || json_object_is_type(value, json_type_null)) {
        return 0;
    }
    if (!json_object_is_type(value, json_type_double) && !json_object_is_type(value, json_type_int)) {
        return 0;
    }

    *out = json_object_get_double(value);
    return 1;
}

// Reads an integer value from a json-c array element as long long.
// Returns 1 on success and stores value in *out, or 0 if null/missing/invalid.
static int yf_json_array_get_int64(struct json_object *array_obj, size_t index, long long *out) {
    if (out) {
        *out = 0;
    }
    if (!array_obj || !out || !json_object_is_type(array_obj, json_type_array)) {
        return 0;
    }

    struct json_object *value = json_object_array_get_idx(array_obj, (int)index);
    if (!value || json_object_is_type(value, json_type_null)) {
        return 0;
    }
    if (!json_object_is_type(value, json_type_int) && !json_object_is_type(value, json_type_double)) {
        return 0;
    }

    *out = (long long)json_object_get_int64(value);
    return 1;
}

// Copies a string field from a json-c object if present.
// Returns nothing.
static void yf_copy_json_string_field(struct json_object *obj, const char *key, char **out_field) {
    if (!out_field) {
        return;
    }

    *out_field = NULL;
    if (!obj || !key) {
        return;
    }

    struct json_object *value = NULL;
    if (!json_object_object_get_ex(obj, key, &value) || !value || !json_object_is_type(value, json_type_string)) {
        return;
    }

    *out_field = yf_strdup_safe(json_object_get_string(value));
}

// Extracts chart error description from Yahoo Finance chart JSON, if present.
// Returns a heap-allocated message string or NULL if unavailable.
static char *yf_extract_chart_error_message(struct json_object *root) {
    if (!root || !json_object_is_type(root, json_type_object)) {
        return NULL;
    }

    struct json_object *chart = NULL;
    if (!json_object_object_get_ex(root, "chart", &chart) || !chart || !json_object_is_type(chart, json_type_object)) {
        return NULL;
    }

    struct json_object *error_obj = NULL;
    if (!json_object_object_get_ex(chart, "error", &error_obj) || !error_obj || json_object_is_type(error_obj, json_type_null)) {
        return NULL;
    }

    if (json_object_is_type(error_obj, json_type_string)) {
        return yf_strdup_safe(json_object_get_string(error_obj));
    }

    if (json_object_is_type(error_obj, json_type_object)) {
        struct json_object *description = NULL;
        if (json_object_object_get_ex(error_obj, "description", &description) && json_object_is_type(description, json_type_string)) {
            return yf_strdup_safe(json_object_get_string(description));
        }
    }

    return yf_strdup_safe("Yahoo Finance chart returned an error");
}

// Parses Yahoo Finance chart JSON into a YFTimeSeries.
// On success, fills *out (caller must free with yf_time_series_free).
// Returns YFOk on success, or a YFErr* status on failure.
YFStatus yf_parse_chart_json(const char *json_data, YFTimeSeries *out) {
    if (!json_data || !out) {
        return YFErrInvalidArg;
    }

    memset(out, 0, sizeof(*out));

    struct json_object *root = json_tokener_parse(json_data);
    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) {
            json_object_put(root);
        }
        return YFErrParse;
    }

    char *chart_error = yf_extract_chart_error_message(root);
    if (chart_error) {
        free(chart_error);
        json_object_put(root);
        return YFErrParse;
    }

    struct json_object *chart = NULL;
    struct json_object *result_array = NULL;
    if (!json_object_object_get_ex(root, "chart", &chart) ||
        !chart || !json_object_is_type(chart, json_type_object) ||
        !json_object_object_get_ex(chart, "result", &result_array) ||
        !result_array || !json_object_is_type(result_array, json_type_array) ||
        json_object_array_length(result_array) < 1) {
        json_object_put(root);
        return YFErrParse;
    }

    struct json_object *result = json_object_array_get_idx(result_array, 0);
    if (!result || !json_object_is_type(result, json_type_object)) {
        json_object_put(root);
        return YFErrParse;
    }

    struct json_object *timestamps = NULL;
    struct json_object *indicators = NULL;
    struct json_object *quote_array = NULL;
    struct json_object *quote = NULL;
    if (!json_object_object_get_ex(result, "timestamp", &timestamps) ||
        !timestamps || !json_object_is_type(timestamps, json_type_array) ||
        !json_object_object_get_ex(result, "indicators", &indicators) ||
        !indicators || !json_object_is_type(indicators, json_type_object) ||
        !json_object_object_get_ex(indicators, "quote", &quote_array) ||
        !quote_array || !json_object_is_type(quote_array, json_type_array) ||
        json_object_array_length(quote_array) < 1) {
        json_object_put(root);
        return YFErrParse;
    }

    quote = json_object_array_get_idx(quote_array, 0);
    if (!quote || !json_object_is_type(quote, json_type_object)) {
        json_object_put(root);
        return YFErrParse;
    }

    struct json_object *opens = NULL;
    struct json_object *highs = NULL;
    struct json_object *lows = NULL;
    struct json_object *closes = NULL;
    struct json_object *volumes = NULL;
    json_object_object_get_ex(quote, "open", &opens);
    json_object_object_get_ex(quote, "high", &highs);
    json_object_object_get_ex(quote, "low", &lows);
    json_object_object_get_ex(quote, "close", &closes);
    json_object_object_get_ex(quote, "volume", &volumes);

    struct json_object *adjclose_array = NULL;
    struct json_object *adjclose_obj = NULL;
    struct json_object *adjcloses = NULL;
    if (json_object_object_get_ex(indicators, "adjclose", &adjclose_array) &&
        adjclose_array && json_object_is_type(adjclose_array, json_type_array) &&
        json_object_array_length(adjclose_array) > 0) {
        adjclose_obj = json_object_array_get_idx(adjclose_array, 0);
        if (adjclose_obj && json_object_is_type(adjclose_obj, json_type_object)) {
            json_object_object_get_ex(adjclose_obj, "adjclose", &adjcloses);
        }
    }

    size_t point_count = (size_t)json_object_array_length(timestamps);
    YFTimeSeriesPoint *points = (YFTimeSeriesPoint *)calloc(point_count, sizeof(YFTimeSeriesPoint));
    if (!points) {
        json_object_put(root);
        return YFErrNomem;
    }

    size_t out_count = 0;
    for (size_t i = 0; i < point_count; i++) {
        struct json_object *ts_value = json_object_array_get_idx(timestamps, (int)i);
        if (!ts_value || (!json_object_is_type(ts_value, json_type_int) && !json_object_is_type(ts_value, json_type_double))) {
            continue;
        }

        double open = 0.0, high = 0.0, low = 0.0, close = 0.0, adjusted_close = 0.0;
        long long volume = 0;
        int has_open = yf_json_array_get_double(opens, i, &open);
        int has_high = yf_json_array_get_double(highs, i, &high);
        int has_low = yf_json_array_get_double(lows, i, &low);
        int has_close = yf_json_array_get_double(closes, i, &close);
        if (!has_open || !has_high || !has_low || !has_close) {
            continue;
        }

        char *date = yf_format_unix_date(json_object_get_int64(ts_value));
        if (!date) {
            continue;
        }

        YFTimeSeriesPoint *point = &points[out_count];
        point->date = date;
        point->timestamp = json_object_get_int64(ts_value);
        point->open = open;
        point->high = high;
        point->low = low;
        point->close = close;
        if (yf_json_array_get_int64(volumes, i, &volume)) {
            point->volume = volume;
        }
        if (yf_json_array_get_double(adjcloses, i, &adjusted_close)) {
            point->adjusted_close = adjusted_close;
            point->has_adjusted_close = 1;
        }
        out_count++;
    }

    struct json_object *meta = NULL;
    if (json_object_object_get_ex(result, "meta", &meta) && meta && json_object_is_type(meta, json_type_object)) {
        yf_copy_json_string_field(meta, "symbol", &out->symbol);
        yf_copy_json_string_field(meta, "currency", &out->currency);
        yf_copy_json_string_field(meta, "exchangeName", &out->exchange_name);
        yf_copy_json_string_field(meta, "instrumentType", &out->instrument_type);
    }

    out->points = points;
    out->count = out_count;

    if (out_count == 0) {
        json_object_put(root);
        yf_time_series_free(out);
        return YFErrParse;
    }

    json_object_put(root);
    return YFOk;
}

// Fetches and parses a chart time series for a symbol.
// interval examples: "1d", "1wk", "1mo". range examples: "1mo", "6mo", "1y", "5y", "max".
// On success, fills *out (caller must free with yf_time_series_free).
// Returns YFOk on success, or a YFErr* status on failure.
YFStatus yf_get_time_series(YFClient *client,
                            const char *symbol,
                            const char *interval,
                            const char *range,
                            YFTimeSeries *out) {
    if (!client || !symbol || !interval || !range || !out) {
        return YFErrInvalidArg;
    }

    memset(out, 0, sizeof(*out));

    YFResponse response;
    yf_response_init(&response);
    YFStatus status = yf_fetch_chart(client, symbol, interval, range, &response);
    if (status != YFOk) {
        yf_response_free(&response);
        return status;
    }

    if (response.http_code != 200 || !response.data) {
        yf_response_free(&response);
        return YFErrCurl;
    }

    status = yf_parse_chart_json(response.data, out);
    yf_response_free(&response);
    return status;
}
