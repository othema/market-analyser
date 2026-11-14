#include "av_search.h"
#include "av_utils.h"

#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

/* Frees all memory owned by an AVSearchResult and resets its fields.
 * Safe to call on an empty or already-freed result. Returns nothing. */
void av_search_result_free(AVSearchResult *result) {
    if (!result) return;
    for (size_t i = 0; i < result->count; i++) {
        AVSearchMatch *m = &result->matches[i];
        free(m->symbol);
        free(m->name);
        free(m->type);
        free(m->region);
        free(m->market_open);
        free(m->market_close);
        free(m->timezone);
        free(m->currency);
        free(m->match_score);
    }
    free(result->matches);
    result->matches = NULL;
    result->count = 0;
}

/* Parses a SYMBOL_SEARCH JSON response body into an AVSearchResult.
 * On success, allocates and fills *out (caller must free with av_search_result_free).
 * Returns AVOk on success, AVErrInvalidArg for bad inputs, AVErrNomem on
 * allocation failure, or AVErrCurl if the content is not a valid search result. */
AVStatus av_parse_search_result(const char *json_data, AVSearchResult *out) {
    if (!json_data || !out) return AVErrInvalidArg;
    out->matches = NULL;
    out->count = 0;
    if (av_response_is_error(json_data)) return AVErrCurl;

    struct json_object *root = json_tokener_parse(json_data);
    if (!root) return AVErrCurl;

    struct json_object *arr = NULL;
    if (!json_object_object_get_ex(root, "bestMatches", &arr) || !json_object_is_type(arr, json_type_array)) {
        json_object_put(root);
        return AVErrCurl;
    }

    size_t n = (size_t)json_object_array_length(arr);
    if (n == 0) {
        json_object_put(root);
        out->matches = NULL;
        out->count = 0;
        return AVOk;
    }

    AVSearchMatch *matches = (AVSearchMatch *)calloc(n, sizeof(AVSearchMatch));
    if (!matches) {
        json_object_put(root);
        return AVErrNomem;
    }

    for (size_t i = 0; i < n; i++) {
        struct json_object *item = json_object_array_get_idx(arr, (int)i);
        AVSearchMatch *m = &matches[i];
        if (!item || !json_object_is_type(item, json_type_object)) continue;
        m->symbol = av_json_get_string_dup(item, "1. symbol");
        m->name = av_json_get_string_dup(item, "2. name");
        m->type = av_json_get_string_dup(item, "3. type");
        m->region = av_json_get_string_dup(item, "4. region");
        m->market_open = av_json_get_string_dup(item, "5. marketOpen");
        m->market_close = av_json_get_string_dup(item, "6. marketClose");
        m->timezone = av_json_get_string_dup(item, "7. timezone");
        m->currency = av_json_get_string_dup(item, "8. currency");
        m->match_score = av_json_get_string_dup(item, "9. matchScore");
    }

    out->matches = matches;
    out->count = n;
    json_object_put(root);
    return AVOk;
}

/* Fetches and parses SYMBOL_SEARCH results for the given keywords.
 * On success, fills *out (caller must free with av_search_result_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_search_symbol(AVClient *client, const char *keywords, AVSearchResult *out) {
    if (!client || !keywords || !out) return AVErrInvalidArg;
    out->matches = NULL; out->count = 0;

    AVParam params[] = { {"keywords", keywords} };
    AVResponse resp;
    av_response_init(&resp);
    AVStatus st = av_query(client, "SYMBOL_SEARCH", params, 1, &resp);
    if (st != AVOk) { av_response_free(&resp); return st; }
    if (resp.http_code != 200 || !resp.data) { av_response_free(&resp); return AVErrCurl; }
    st = av_parse_search_result(resp.data, out);
    av_response_free(&resp);
    return st;
}
