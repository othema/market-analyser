#include "av_utils.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Duplicates a NUL-terminated string onto the heap.
 * Returns a newly allocated copy on success, or NULL if s is NULL or allocation fails.
 * The caller is responsible for freeing the returned string. */
char *av_strdup_safe(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len + 1);
    return out;
}

/* Checks whether a raw API response body indicates an API-level error or notice.
 * Detects common Alpha Vantage JSON payloads such as {"Error Message":...},
 * {"Note":...}, and {"Information":...}.
 * Returns 1 if an error/note/information payload is detected, otherwise 0. */
int av_response_is_error(const char *data) {
    if (!data) return 1;
    while (*data && isspace((unsigned char)*data)) data++;
    if (*data != '{') return 0;

    struct json_object *root = json_tokener_parse(data);
    if (!root) return 0;
    int is_err = 0;
    if (json_object_object_get_ex(root, "Error Message", NULL)) is_err = 1;
    else if (json_object_object_get_ex(root, "Note", NULL)) is_err = 1;
    else if (json_object_object_get_ex(root, "Information", NULL)) is_err = 1;
    json_object_put(root);
    return is_err;
}

/* Extracts a human-readable error/note/information message from a raw API response.
 * If the response is a JSON object containing "Error Message", "Note", or
 * "Information", returns a heap-allocated copy of that message. Otherwise returns
 * a heap-allocated copy of a generic message (or NULL on allocation failure).
 * The caller is responsible for freeing the returned string. */
char *av_response_error_message(const char *data) {
    if (!data) return av_strdup_safe("Empty response");
    const char *trim = data;
    while (*trim && isspace((unsigned char)*trim)) trim++;
    if (*trim == '{') {
        struct json_object *root = json_tokener_parse(trim);
        if (root) {
            struct json_object *val = NULL;
            const char *msg = NULL;
            if (json_object_object_get_ex(root, "Error Message", &val)) msg = json_object_get_string(val);
            else if (json_object_object_get_ex(root, "Note", &val)) msg = json_object_get_string(val);
            else if (json_object_object_get_ex(root, "Information", &val)) msg = json_object_get_string(val);
            char *out = msg ? av_strdup_safe(msg) : av_strdup_safe("Unknown API error response");
            json_object_put(root);
            return out;
        }
    }
    return av_strdup_safe("Unrecognized API response");
}

/* Retrieves a string value for a key from a json-c object.
 * Returns a heap-allocated copy of the string value if present and of string type,
 * otherwise NULL. The caller is responsible for freeing the returned string. */
char *av_json_get_string_dup(struct json_object *obj, const char *key) {
    if (!obj || !key) return NULL;
    struct json_object *val = NULL;
    if (!json_object_object_get_ex(obj, key, &val) || !val) return NULL;
    if (!json_object_is_type(val, json_type_string)) return NULL;
    const char *s = json_object_get_string(val);
    return av_strdup_safe(s);
}

/* Retrieves a string representation of a value for a key from a json-c object.
 * Converts string, int, and double values to a newly allocated string; returns
 * NULL if the key is missing or cannot be represented. The caller is responsible
 * for freeing the returned string. */
char *av_json_get_value_string_dup(struct json_object *obj, const char *key) {
    if (!obj || !key) return NULL;
    struct json_object *val = NULL;
    if (!json_object_object_get_ex(obj, key, &val) || !val) return NULL;
    if (json_object_is_type(val, json_type_string)) {
        return av_strdup_safe(json_object_get_string(val));
    }
    if (json_object_is_type(val, json_type_int) || json_object_is_type(val, json_type_double)) {
        const char *s = json_object_get_string(val);
        return av_strdup_safe(s);
    }
    return NULL;
}

/* Parses a NUL-terminated string as a double.
 * Returns 1 on success and stores the result in *out; returns 0 on failure or if
 * inputs are NULL/empty/non-numeric (out is set to 0.0 on failure). */
int av_parse_double(const char *s, double *out) {
    if (out) *out = 0.0;
    if (!s || !out) return 0;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return 0;
    errno = 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (errno != 0 || end == s) return 0;
    while (*end && isspace((unsigned char)*end)) end++;
    if (*end != '\0') return 0;
    *out = v;
    return 1;
}

/* Parses a NUL-terminated string as a long long integer.
 * Returns 1 on success and stores the result in *out; returns 0 on failure or if
 * inputs are NULL/empty/non-numeric (out is set to 0 on failure). */
int av_parse_long_long(const char *s, long long *out) {
    if (out) *out = 0;
    if (!s || !out) return 0;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return 0;
    errno = 0;
    char *end = NULL;
    long long v = strtoll(s, &end, 10);
    if (errno != 0 || end == s) return 0;
    while (*end && isspace((unsigned char)*end)) end++;
    if (*end != '\0') return 0;
    *out = v;
    return 1;
}
