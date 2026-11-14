#include "av_time_series.h"
#include "av_utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Frees all memory owned by an AVTimeSeries and resets its fields.
// Safe to call on an empty or already-freed series. Returns nothing.
void av_time_series_free(AVTimeSeries *series) {
    if (!series) {
        return;
    }

    for (size_t i = 0; i < series->count; i++) {
        free(series->points[i].date);
        series->points[i].date = NULL;
    }
    free(series->points);
    series->points = NULL;
    series->count = 0;
}

// Trims leading and trailing whitespace from a string in place.
// Returns a pointer to the trimmed start of the string.
static char *av_trim_in_place(char *text) {
    if (!text) {
        return text;
    }

    while (*text && isspace((unsigned char)*text)) {
        text++;
    }

    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return text;
}

// Splits a CSV line in place by commas into at most max_fields fields.
// Does not handle quoted commas. NUL-terminates each field.
// Returns the number of fields found (capped at max_fields).
static size_t av_split_csv_line(char *line, char **fields, size_t max_fields) {
    if (!line || !fields || max_fields == 0) {
        return 0;
    }

    size_t count = 0;
    char *cursor = line;
    fields[count++] = cursor;

    while (*cursor && count < max_fields) {
        if (*cursor == ',') {
            *cursor = '\0';
            fields[count++] = cursor + 1;
        }
        cursor++;
    }

    // Truncate any extra comma-separated content in the last accepted field.
    if (*cursor) {
        while (*cursor) {
            if (*cursor == ',') {
                *cursor = '\0';
                break;
            }
            cursor++;
        }
    }

    return count;
}

// Maps a lowercased time-series CSV header name to an internal column role.
// Returns 0=date,1=open,2=high,3=low,4=close,5=adjusted_close,6=volume,
// 7=dividend_amount,8=split_coefficient, or -1 for unknown columns.
static int av_time_series_header_role(const char *name) {
    if (!name) {
        return -1;
    }
    if (strcmp(name, "timestamp") == 0 || strcmp(name, "date") == 0 || strcmp(name, "time") == 0) {
        return 0;
    }
    if (strcmp(name, "open") == 0) {
        return 1;
    }
    if (strcmp(name, "high") == 0) {
        return 2;
    }
    if (strcmp(name, "low") == 0) {
        return 3;
    }
    if (strcmp(name, "close") == 0) {
        return 4;
    }
    if (strcmp(name, "adjusted close") == 0 || strcmp(name, "adjusted_close") == 0) {
        return 5;
    }
    if (strcmp(name, "volume") == 0) {
        return 6;
    }
    if (strcmp(name, "dividend amount") == 0 || strcmp(name, "dividend_amount") == 0) {
        return 7;
    }
    if (strcmp(name, "split coefficient") == 0 || strcmp(name, "split_coefficient") == 0) {
        return 8;
    }
    return -1;
}

// Converts an ASCII string to lowercase in place.
// Returns nothing.
static void av_to_lower_ascii_in_place(char *text) {
    if (!text) {
        return;
    }
    for (char *cursor = text; *cursor; ++cursor) {
        *cursor = (char)tolower((unsigned char)*cursor);
    }
}

// Detects header column indices for supported time-series fields.
// Returns 1 if required columns (date and close) are present, otherwise 0.
static int av_detect_time_series_columns(char *header_line,
                                         int *date_idx,
                                         int *open_idx,
                                         int *high_idx,
                                         int *low_idx,
                                         int *close_idx,
                                         int *adj_idx,
                                         int *volume_idx,
                                         int *div_idx,
                                         int *split_idx) {
    char *fields[16];
    size_t field_count = av_split_csv_line(header_line, fields, 16);

    *date_idx = -1;
    *open_idx = -1;
    *high_idx = -1;
    *low_idx = -1;
    *close_idx = -1;
    *adj_idx = -1;
    *volume_idx = -1;
    *div_idx = -1;
    *split_idx = -1;

    for (size_t i = 0; i < field_count; i++) {
        char *name = av_trim_in_place(fields[i]);
        av_to_lower_ascii_in_place(name);
        int role = av_time_series_header_role(name);
        if (role == 0) *date_idx = (int)i;
        else if (role == 1) *open_idx = (int)i;
        else if (role == 2) *high_idx = (int)i;
        else if (role == 3) *low_idx = (int)i;
        else if (role == 4) *close_idx = (int)i;
        else if (role == 5) *adj_idx = (int)i;
        else if (role == 6) *volume_idx = (int)i;
        else if (role == 7) *div_idx = (int)i;
        else if (role == 8) *split_idx = (int)i;
    }

    return (*date_idx >= 0 && *close_idx >= 0);
}

// Parses a single CSV data row into a time-series point.
// Returns 1 on success, or 0 if the row is unusable (for example missing date).
static int av_parse_time_series_row(char *line,
                                    int date_idx,
                                    int open_idx,
                                    int high_idx,
                                    int low_idx,
                                    int close_idx,
                                    int adj_idx,
                                    int volume_idx,
                                    int div_idx,
                                    int split_idx,
                                    AVTimeSeriesPoint *out_point) {
    char *fields[16];
    size_t field_count = av_split_csv_line(line, fields, 16);
    memset(out_point, 0, sizeof(*out_point));

    if (date_idx < 0 || (size_t)date_idx >= field_count) {
        return 0;
    }

    out_point->date = av_strdup_safe(av_trim_in_place(fields[date_idx]));
    if (!out_point->date) {
        return 0;
    }

    if (open_idx >= 0 && (size_t)open_idx < field_count) {
        av_parse_double(av_trim_in_place(fields[open_idx]), &out_point->open);
    }
    if (high_idx >= 0 && (size_t)high_idx < field_count) {
        av_parse_double(av_trim_in_place(fields[high_idx]), &out_point->high);
    }
    if (low_idx >= 0 && (size_t)low_idx < field_count) {
        av_parse_double(av_trim_in_place(fields[low_idx]), &out_point->low);
    }
    if (close_idx >= 0 && (size_t)close_idx < field_count) {
        av_parse_double(av_trim_in_place(fields[close_idx]), &out_point->close);
    }
    if (adj_idx >= 0 && (size_t)adj_idx < field_count) {
        out_point->has_adjusted_close = av_parse_double(av_trim_in_place(fields[adj_idx]), &out_point->adjusted_close);
    }
    if (volume_idx >= 0 && (size_t)volume_idx < field_count) {
        av_parse_long_long(av_trim_in_place(fields[volume_idx]), &out_point->volume);
    }
    if (div_idx >= 0 && (size_t)div_idx < field_count) {
        out_point->has_dividend_amount = av_parse_double(av_trim_in_place(fields[div_idx]), &out_point->dividend_amount);
    }
    if (split_idx >= 0 && (size_t)split_idx < field_count) {
        out_point->has_split_coefficient = av_parse_double(av_trim_in_place(fields[split_idx]), &out_point->split_coefficient);
    }

    return 1;
}

// Parses a time series from a raw Alpha Vantage CSV response body.
// On success, fills *out (caller must free with av_time_series_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_parse_time_series_csv(const char *csv_data, AVTimeSeries *out) {
    if (!csv_data || !out) {
        return AVErrInvalidArg;
    }

    out->points = NULL;
    out->count = 0;

    const char *trimmed = csv_data;
    while (*trimmed && isspace((unsigned char)*trimmed)) {
        trimmed++;
    }
    if (*trimmed == '{' || av_response_is_error(csv_data)) {
        return AVErrCurl;
    }

    char *buffer = av_strdup_safe(csv_data);
    if (!buffer) {
        return AVErrNomem;
    }

    // Collect non-empty line pointers in place.
    size_t line_capacity = 0;
    size_t line_count = 0;
    char **lines = NULL;
    char *line_start = buffer;
    for (char *cursor = buffer;; cursor++) {
        if (*cursor == '\n' || *cursor == '\r' || *cursor == '\0') {
            char delimiter = *cursor;
            *cursor = '\0';
            char *line = av_trim_in_place(line_start);
            if (*line != '\0') {
                if (line_count + 1 > line_capacity) {
                    size_t new_capacity = line_capacity ? line_capacity * 2 : 128;
                    char **new_lines = (char **)realloc(lines, new_capacity * sizeof(char *));
                    if (!new_lines) {
                        free(lines);
                        free(buffer);
                        return AVErrNomem;
                    }
                    lines = new_lines;
                    line_capacity = new_capacity;
                }
                lines[line_count++] = line;
            }
            if (delimiter == '\0') {
                break;
            }
            if (delimiter == '\r' && *(cursor + 1) == '\n') {
                cursor++;
            }
            line_start = cursor + 1;
        }
    }

    if (line_count < 2) {
        free(lines);
        free(buffer);
        return AVErrCurl;
    }

    char *header_line = av_strdup_safe(lines[0]);
    if (!header_line) {
        free(lines);
        free(buffer);
        return AVErrNomem;
    }

    int date_idx, open_idx, high_idx, low_idx, close_idx, adj_idx, volume_idx, div_idx, split_idx;
    int has_required_columns = av_detect_time_series_columns(
        header_line,
        &date_idx, &open_idx, &high_idx, &low_idx, &close_idx,
        &adj_idx, &volume_idx, &div_idx, &split_idx
    );
    free(header_line);
    if (!has_required_columns) {
        free(lines);
        free(buffer);
        return AVErrCurl;
    }

    size_t point_capacity = line_count - 1;
    AVTimeSeriesPoint *points = (AVTimeSeriesPoint *)calloc(point_capacity, sizeof(AVTimeSeriesPoint));
    if (!points) {
        free(lines);
        free(buffer);
        return AVErrNomem;
    }

    size_t parsed_count = 0;
    for (size_t i = 1; i < line_count; i++) {
        char *row = av_strdup_safe(lines[i]);
        if (!row) {
            out->points = points;
            out->count = parsed_count;
            av_time_series_free(out);
            free(lines);
            free(buffer);
            return AVErrNomem;
        }

        AVTimeSeriesPoint point;
        if (av_parse_time_series_row(row, date_idx, open_idx, high_idx, low_idx, close_idx,
                                     adj_idx, volume_idx, div_idx, split_idx, &point)) {
            points[parsed_count++] = point;
        } else {
            free(point.date);
        }
        free(row);
    }

    free(lines);
    free(buffer);

    out->points = points;
    out->count = parsed_count;
    return AVOk;
}
