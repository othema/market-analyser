#include "av_utils.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// Duplicates a NUL-terminated string onto the heap.
// Returns a newly allocated copy on success, or NULL if s is NULL or allocation fails.
// The caller is responsible for freeing the returned string.
char *av_strdup_safe(const char *s) {
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

// Checks whether a raw API response body indicates an API-level error or notice.
// Detects common Alpha Vantage JSON payloads such as {"Error Message":...},
// {"Note":...}, and {"Information":...}.
// Returns 1 if an error/note/information payload is detected, otherwise 0.
int av_response_is_error(const char *data) {
    if (!data) {
        return 1;
    }

    while (*data && isspace((unsigned char)*data)) {
        data++;
    }
    if (*data != '{') {
        return 0;
    }

    struct json_object *root = json_tokener_parse(data);
    if (!root) {
        return 0;
    }

    int is_error = 0;
    if (json_object_object_get_ex(root, "Error Message", NULL)) {
        is_error = 1;
    } else if (json_object_object_get_ex(root, "Note", NULL)) {
        is_error = 1;
    } else if (json_object_object_get_ex(root, "Information", NULL)) {
        is_error = 1;
    }

    json_object_put(root);
    return is_error;
}

// Extracts a human-readable error/note/information message from a raw API response.
// Returns a heap-allocated message string, or NULL on allocation failure.
// The caller is responsible for freeing the returned string.
char *av_response_error_message(const char *data) {
    if (!data) {
        return av_strdup_safe("Empty response");
    }

    const char *trimmed = data;
    while (*trimmed && isspace((unsigned char)*trimmed)) {
        trimmed++;
    }

    if (*trimmed == '{') {
        struct json_object *root = json_tokener_parse(trimmed);
        if (root) {
            struct json_object *value = NULL;
            const char *message = NULL;
            if (json_object_object_get_ex(root, "Error Message", &value)) {
                message = json_object_get_string(value);
            } else if (json_object_object_get_ex(root, "Note", &value)) {
                message = json_object_get_string(value);
            } else if (json_object_object_get_ex(root, "Information", &value)) {
                message = json_object_get_string(value);
            }

            char *result = message ? av_strdup_safe(message) : av_strdup_safe("Unknown API error response");
            json_object_put(root);
            return result;
        }
    }

    return av_strdup_safe("Unrecognized API response");
}

// Retrieves a string value for a key from a json-c object.
// Returns a heap-allocated copy of the string value if present, otherwise NULL.
// The caller is responsible for freeing the returned string.
char *av_json_get_string_dup(struct json_object *obj, const char *key) {
    if (!obj || !key) {
        return NULL;
    }

    struct json_object *value = NULL;
    if (!json_object_object_get_ex(obj, key, &value) || !value) {
        return NULL;
    }
    if (!json_object_is_type(value, json_type_string)) {
        return NULL;
    }

    return av_strdup_safe(json_object_get_string(value));
}

// Retrieves a string representation of a value for a key from a json-c object.
// Supports string, int, and double values.
// Returns a heap-allocated string on success, otherwise NULL.
// The caller is responsible for freeing the returned string.
char *av_json_get_value_string_dup(struct json_object *obj, const char *key) {
    if (!obj || !key) {
        return NULL;
    }

    struct json_object *value = NULL;
    if (!json_object_object_get_ex(obj, key, &value) || !value) {
        return NULL;
    }

    if (json_object_is_type(value, json_type_string) ||
        json_object_is_type(value, json_type_int) ||
        json_object_is_type(value, json_type_double)) {
        return av_strdup_safe(json_object_get_string(value));
    }

    return NULL;
}

// Parses a NUL-terminated string as a double.
// Returns 1 on success and stores the result in *out; returns 0 on failure
// and sets *out to 0.0 when out is not NULL.
int av_parse_double(const char *s, double *out) {
    if (out) {
        *out = 0.0;
    }
    if (!s || !out) {
        return 0;
    }

    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    if (*s == '\0') {
        return 0;
    }

    errno = 0;
    char *end = NULL;
    double value = strtod(s, &end);
    if (errno != 0 || end == s) {
        return 0;
    }
    while (*end && isspace((unsigned char)*end)) {
        end++;
    }
    if (*end != '\0') {
        return 0;
    }

    *out = value;
    return 1;
}

// Parses a NUL-terminated string as a long long integer.
// Returns 1 on success and stores the result in *out; returns 0 on failure
// and sets *out to 0 when out is not NULL.
int av_parse_long_long(const char *s, long long *out) {
    if (out) {
        *out = 0;
    }
    if (!s || !out) {
        return 0;
    }

    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    if (*s == '\0') {
        return 0;
    }

    errno = 0;
    char *end = NULL;
    long long value = strtoll(s, &end, 10);
    if (errno != 0 || end == s) {
        return 0;
    }
    while (*end && isspace((unsigned char)*end)) {
        end++;
    }
    if (*end != '\0') {
        return 0;
    }

    *out = value;
    return 1;
}

