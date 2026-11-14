#include "av_time_series.h"
#include "av_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Frees all memory owned by an AVTimeSeries and resets its fields.
 * Safe to call on an empty or already-freed series. Returns nothing. */
void av_time_series_free(AVTimeSeries *series) {
    if (!series) return;
    for (size_t i = 0; i < series->count; i++) {
        free(series->points[i].date);
        series->points[i].date = NULL;
    }
    free(series->points);
    series->points = NULL;
    series->count = 0;
}

/* Trims leading and trailing whitespace/newlines from a string in place.
 * Returns the pointer to the trimmed start of the string. */
static char *av_trim(char *s) {
    if (!s) return s;
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return s;
}

/* Splits a CSV line in place by commas into fields (simple split, no quoted-comma handling).
 * Stores pointers to the start of each field in fields[0..max_fields-1] and NUL-terminates them.
 * Returns the number of fields found (capped at max_fields). */
static size_t av_split_csv_line(char *line, char **fields, size_t max_fields) {
    size_t n = 0;
    if (!line || !fields || max_fields == 0) return 0;
    char *p = line;
    fields[n++] = p;
    while (*p && n < max_fields) {
        if (*p == ',') {
            *p = '\0';
            fields[n++] = p + 1;
        }
        p++;
    }
    /* if more commas exist beyond max_fields, truncate last field at next comma */
    if (*p) {
        while (*p) {
            if (*p == ',') { *p = '\0'; break; }
            p++;
        }
    }
    return n;
}

/* Maps a time-series CSV header name to a column index role.
 * Returns an integer code: 0=date/timestamp,1=open,2=high,3=low,4=close,
 * 5=adjusted_close,6=volume,7=dividend_amount,8=split_coefficient, -1 unknown. */
static int av_ts_header_role(const char *name) {
    if (!name) return -1;
    if (strcmp(name, "timestamp") == 0 || strcmp(name, "date") == 0 || strcmp(name, "time") == 0) return 0;
    if (strcmp(name, "open") == 0) return 1;
    if (strcmp(name, "high") == 0) return 2;
    if (strcmp(name, "low") == 0) return 3;
    if (strcmp(name, "close") == 0) return 4;
    if (strcmp(name, "adjusted close") == 0 || strcmp(name, "adjusted_close") == 0) return 5;
    if (strcmp(name, "volume") == 0) return 6;
    if (strcmp(name, "dividend amount") == 0 || strcmp(name, "dividend_amount") == 0) return 7;
    if (strcmp(name, "split coefficient") == 0 || strcmp(name, "split_coefficient") == 0) return 8;
    return -1;
}

/* Parses a time series from a raw Alpha Vantage CSV response body.
 * Supports daily/weekly/monthly series in both raw and adjusted forms by
 * detecting columns from the header row. On success, allocates and fills *out
 * (caller must free with av_time_series_free).
 * Returns AVOk on success, AVErrInvalidArg for bad inputs, AVErrNomem on
 * allocation failure, or AVErrCurl if the content is not a valid parseable
 * time series CSV (e.g. an API error payload). */
AVStatus av_parse_time_series_csv(const char *csv_data, AVTimeSeries *out) {
    if (!csv_data || !out) return AVErrInvalidArg;
    out->points = NULL;
    out->count = 0;

    /* Detect API error/note JSON responses. */
    const char *trimmed = csv_data;
    while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
    if (*trimmed == '{' || av_response_is_error(csv_data)) {
        return AVErrCurl;
    }

    /* Work on a mutable copy. */
    char *buf = av_strdup_safe(csv_data);
    if (!buf) return AVErrNomem;

    /* Collect line starts. */
    size_t lines_cap = 0, lines_n = 0;
    char **lines = NULL;
    char *save = buf;
    for (char *p = buf; ; p++) {
        if (*p == '\n' || *p == '\r' || *p == '\0') {
            char delim = *p;
            *p = '\0';
            char *ls = av_trim(save);
            if (*ls != '\0') {
                if (lines_n + 1 > lines_cap) {
                    size_t nc = lines_cap ? lines_cap * 2 : 128;
                    char **nl = (char **)realloc(lines, nc * sizeof(char *));
                    if (!nl) { free(lines); free(buf); return AVErrNomem; }
                    lines = nl; lines_cap = nc;
                }
                lines[lines_n++] = ls;
            }
            if (delim == '\0') break;
            /* handle \r\n */
            if (delim == '\r' && *(p + 1) == '\n') p++;
            save = p + 1;
        }
    }

    if (lines_n < 2) {
        free(lines); free(buf);
        return AVErrCurl;
    }

    /* Parse header. */
    char *header_fields_raw[16];
    char *header_line = av_strdup_safe(lines[0]);
    if (!header_line) { free(lines); free(buf); return AVErrNomem; }
    size_t hcount = av_split_csv_line(header_line, header_fields_raw, 16);
    int idx_date=-1, idx_open=-1, idx_high=-1, idx_low=-1, idx_close=-1, idx_adj=-1, idx_vol=-1, idx_div=-1, idx_split=-1;
    for (size_t i = 0; i < hcount; i++) {
        char *h = av_trim(header_fields_raw[i]);
        /* lowercase copy for matching */
        for (char *c = h; *c; ++c) *c = (char)tolower((unsigned char)*c);
        int role = av_ts_header_role(h);
        if (role == 0) idx_date = (int)i;
        else if (role == 1) idx_open = (int)i;
        else if (role == 2) idx_high = (int)i;
        else if (role == 3) idx_low = (int)i;
        else if (role == 4) idx_close = (int)i;
        else if (role == 5) idx_adj = (int)i;
        else if (role == 6) idx_vol = (int)i;
        else if (role == 7) idx_div = (int)i;
        else if (role == 8) idx_split = (int)i;
    }
    free(header_line);

    if (idx_date < 0 || idx_close < 0) {
        free(lines); free(buf);
        return AVErrCurl;
    }

    size_t npts = lines_n - 1;
    AVTimeSeriesPoint *pts = (AVTimeSeriesPoint *)calloc(npts, sizeof(AVTimeSeriesPoint));
    if (!pts) { free(lines); free(buf); return AVErrNomem; }

    size_t out_n = 0;
    for (size_t li = 1; li < lines_n; li++) {
        char *line = av_strdup_safe(lines[li]);
        if (!line) { /* cleanup partial */
            out->points = pts; out->count = out_n;
            av_time_series_free(out);
            free(lines); free(buf);
            return AVErrNomem;
        }
        char *fields[16];
        size_t fcount = av_split_csv_line(line, fields, 16);
        AVTimeSeriesPoint *pt = &pts[out_n];
        memset(pt, 0, sizeof(*pt));
        if (idx_date >= 0 && (size_t)idx_date < fcount) pt->date = av_strdup_safe(av_trim(fields[idx_date]));
        if (!pt->date) { free(line); continue; }
        if (idx_open >= 0 && (size_t)idx_open < fcount) av_parse_double(av_trim(fields[idx_open]), &pt->open);
        if (idx_high >= 0 && (size_t)idx_high < fcount) av_parse_double(av_trim(fields[idx_high]), &pt->high);
        if (idx_low >= 0 && (size_t)idx_low < fcount) av_parse_double(av_trim(fields[idx_low]), &pt->low);
        if (idx_close >= 0 && (size_t)idx_close < fcount) av_parse_double(av_trim(fields[idx_close]), &pt->close);
        if (idx_adj >= 0 && (size_t)idx_adj < fcount) { pt->has_adjusted_close = av_parse_double(av_trim(fields[idx_adj]), &pt->adjusted_close); }
        if (idx_vol >= 0 && (size_t)idx_vol < fcount) av_parse_long_long(av_trim(fields[idx_vol]), &pt->volume);
        if (idx_div >= 0 && (size_t)idx_div < fcount) { pt->has_dividend_amount = av_parse_double(av_trim(fields[idx_div]), &pt->dividend_amount); }
        if (idx_split >= 0 && (size_t)idx_split < fcount) { pt->has_split_coefficient = av_parse_double(av_trim(fields[idx_split]), &pt->split_coefficient); }
        out_n++;
        free(line);
    }

    free(lines);
    free(buf);
    out->points = pts;
    out->count = out_n;
    return AVOk;
}

/* Internal helper that fetches a time-series function as CSV and parses it.
 * Returns AVOk on success with *out filled (caller frees with av_time_series_free),
 * or an AVErr* status on transport/API/parse failure. */
static AVStatus av_get_time_series_common(AVClient *client, const char *function, const char *symbol, AVTimeSeries *out) {
    if (!client || !function || !symbol || !out) return AVErrInvalidArg;
    out->points = NULL; out->count = 0;

    AVParam params[] = {
        {"symbol", symbol},
        {"datatype", "csv"},
        {"outputsize", "compact"}
    };
    AVResponse resp;
    av_response_init(&resp);
    AVStatus st = av_query(client, function, params, 3, &resp);
    if (st != AVOk) {
        av_response_free(&resp);
        return st;
    }
    if (resp.http_code != 200 || !resp.data || av_response_is_error(resp.data)) {
        av_response_free(&resp);
        return AVErrCurl;
    }
    st = av_parse_time_series_csv(resp.data, out);
    av_response_free(&resp);
    return st;
}

/* Fetches and parses a daily raw time series (TIME_SERIES_DAILY) for a symbol.
 * Requests CSV (compact). On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_daily(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_DAILY", symbol, out);
}

/* Fetches and parses a daily adjusted time series (TIME_SERIES_DAILY_ADJUSTED).
 * Requests CSV (compact). On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_daily_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_DAILY_ADJUSTED", symbol, out);
}

/* Fetches and parses a weekly raw time series (TIME_SERIES_WEEKLY).
 * Requests CSV. On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_weekly(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_WEEKLY", symbol, out);
}

/* Fetches and parses a weekly adjusted time series (TIME_SERIES_WEEKLY_ADJUSTED).
 * Requests CSV. On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_weekly_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_WEEKLY_ADJUSTED", symbol, out);
}

/* Fetches and parses a monthly raw time series (TIME_SERIES_MONTHLY).
 * Requests CSV. On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_monthly(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_MONTHLY", symbol, out);
}

/* Fetches and parses a monthly adjusted time series (TIME_SERIES_MONTHLY_ADJUSTED).
 * Requests CSV. On success fills *out (caller frees with av_time_series_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_time_series_monthly_adjusted(AVClient *client, const char *symbol, AVTimeSeries *out) {
    return av_get_time_series_common(client, "TIME_SERIES_MONTHLY_ADJUSTED", symbol, out);
}
