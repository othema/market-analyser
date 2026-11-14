#include "av_search.h"
#include "av_utils.h"

#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

// Frees all memory owned by an AVSearchResult and resets its fields.
// Safe to call on an empty or already-freed result. Returns nothing.
void av_search_result_free(AVSearchResult *result) {
    if (!result) {
        return;
    }

    for (size_t i = 0; i < result->count; i++) {
        AVSearchMatch *match = &result->matches[i];
        free(match->symbol);
        free(match->name);
        free(match->type);
        free(match->region);
        free(match->market_open);
        free(match->market_close);
        free(match->timezone);
        free(match->currency);
        free(match->match_score);
    }

    free(result->matches);
    result->matches = NULL;
    result->count = 0;
}

// Parses a SYMBOL_SEARCH JSON response body into an AVSearchResult.
// On success, fills *out (caller must free with av_search_result_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_parse_search_result(const char *json_data, AVSearchResult *out) {
    if (!json_data || !out) {
        return AVErrInvalidArg;
    }

    out->matches = NULL;
    out->count = 0;
    if (av_response_is_error(json_data)) {
        return AVErrCurl;
    }

    struct json_object *root = json_tokener_parse(json_data);
    if (!root) {
        return AVErrCurl;
    }

    struct json_object *matches_array = NULL;
    if (!json_object_object_get_ex(root, "bestMatches", &matches_array) ||
        !json_object_is_type(matches_array, json_type_array)) {
        json_object_put(root);
        return AVErrCurl;
    }

    size_t count = (size_t)json_object_array_length(matches_array);
    if (count == 0) {
        json_object_put(root);
        return AVOk;
    }

    AVSearchMatch *matches = (AVSearchMatch *)calloc(count, sizeof(AVSearchMatch));
    if (!matches) {
        json_object_put(root);
        return AVErrNomem;
    }

    for (size_t i = 0; i < count; i++) {
        struct json_object *item = json_object_array_get_idx(matches_array, (int)i);
        if (!item || !json_object_is_type(item, json_type_object)) {
            continue;
        }

        AVSearchMatch *match = &matches[i];
        match->symbol = av_json_get_string_dup(item, "1. symbol");
        match->name = av_json_get_string_dup(item, "2. name");
        match->type = av_json_get_string_dup(item, "3. type");
        match->region = av_json_get_string_dup(item, "4. region");
        match->market_open = av_json_get_string_dup(item, "5. marketOpen");
        match->market_close = av_json_get_string_dup(item, "6. marketClose");
        match->timezone = av_json_get_string_dup(item, "7. timezone");
        match->currency = av_json_get_string_dup(item, "8. currency");
        match->match_score = av_json_get_string_dup(item, "9. matchScore");
    }

    out->matches = matches;
    out->count = count;
    json_object_put(root);
    return AVOk;
}

// Fetches and parses SYMBOL_SEARCH results for the given keywords.
// On success, fills *out (caller must free with av_search_result_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_search_symbol(AVClient *client, const char *keywords, AVSearchResult *out) {
    if (!client || !keywords || !out) {
        return AVErrInvalidArg;
    }

    out->matches = NULL;
    out->count = 0;

    AVParam params[] = {{"keywords", keywords}};
    AVResponse response;
    av_response_init(&response);

    AVStatus status = av_query(client, "SYMBOL_SEARCH", params, 1, &response);
    if (status != AVOk) {
        av_response_free(&response);
        return status;
    }
    if (response.http_code != 200 || !response.data) {
        av_response_free(&response);
        return AVErrCurl;
    }

    status = av_parse_search_result(response.data, out);
    av_response_free(&response);
    return status;
}
