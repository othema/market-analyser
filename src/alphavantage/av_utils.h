#ifndef AV_UTILS_H
#define AV_UTILS_H

#include <json-c/json.h>

#ifdef __cplusplus
extern "C" {
#endif

// Duplicates a NUL-terminated string onto the heap.
// Returns a newly allocated copy on success, or NULL if s is NULL or allocation fails.
// The caller is responsible for freeing the returned string.
char *av_strdup_safe(const char *s);

// Checks whether a raw API response body indicates an API-level error or notice.
// Detects common Alpha Vantage JSON payloads such as {"Error Message":...},
// {"Note":...}, and {"Information":...}.
// Returns 1 if an error/note/information payload is detected, otherwise 0.
int av_response_is_error(const char *data);

// Extracts a human-readable error/note/information message from a raw API response.
// Returns a heap-allocated message string, or NULL on allocation failure.
// The caller is responsible for freeing the returned string.
char *av_response_error_message(const char *data);

// Retrieves a string value for a key from a json-c object.
// Returns a heap-allocated copy of the string value if present, otherwise NULL.
// The caller is responsible for freeing the returned string.
char *av_json_get_string_dup(struct json_object *obj, const char *key);

// Retrieves a string representation of a value for a key from a json-c object.
// Supports string, int, and double values.
// Returns a heap-allocated string on success, otherwise NULL.
// The caller is responsible for freeing the returned string.
char *av_json_get_value_string_dup(struct json_object *obj, const char *key);

// Parses a NUL-terminated string as a double.
// Returns 1 on success and stores the result in *out; returns 0 on failure
// and sets *out to 0.0 when out is not NULL.
int av_parse_double(const char *s, double *out);

// Parses a NUL-terminated string as a long long integer.
// Returns 1 on success and stores the result in *out; returns 0 on failure
// and sets *out to 0 when out is not NULL.
int av_parse_long_long(const char *s, long long *out);

#ifdef __cplusplus
}
#endif

#endif // AV_UTILS_H
