#include "av_market.h"
#include "av_utils.h"

#include <json-c/json.h>
#include <stdlib.h>

/* Frees all memory owned by an AVMarketStatus and resets its fields.
 * Safe to call on an empty or already-freed status. Returns nothing. */
void av_market_status_free(AVMarketStatus *status) {
    if (!status) return;
    free(status->endpoint);
    for (size_t i = 0; i < status->count; i++) {
        AVMarketInfo *m = &status->markets[i];
        free(m->market_type);
        free(m->region);
        free(m->primary_exchanges);
        free(m->local_open);
        free(m->local_close);
        free(m->current_status);
        free(m->notes);
    }
    free(status->markets);
    status->endpoint = NULL;
    status->markets = NULL;
    status->count = 0;
}

/* Parses a MARKET_STATUS JSON response body into an AVMarketStatus.
 * On success, allocates and fills *out (caller must free with av_market_status_free).
 * Returns AVOk on success, AVErrInvalidArg for bad inputs, AVErrNomem on
 * allocation failure, or AVErrCurl if the content is not a valid market status. */
AVStatus av_parse_market_status(const char *json_data, AVMarketStatus *out) {
    if (!json_data || !out) return AVErrInvalidArg;
    out->endpoint = NULL; out->markets = NULL; out->count = 0;
    if (av_response_is_error(json_data)) return AVErrCurl;

    struct json_object *root = json_tokener_parse(json_data);
    if (!root) return AVErrCurl;

    struct json_object *arr = NULL;
    if (!json_object_object_get_ex(root, "markets", &arr) || !json_object_is_type(arr, json_type_array)) {
        json_object_put(root);
        return AVErrCurl;
    }

    out->endpoint = av_json_get_string_dup(root, "endpoint");

    size_t n = (size_t)json_object_array_length(arr);
    if (n == 0) {
        json_object_put(root);
        return AVOk;
    }

    AVMarketInfo *markets = (AVMarketInfo *)calloc(n, sizeof(AVMarketInfo));
    if (!markets) {
        json_object_put(root);
        av_market_status_free(out);
        return AVErrNomem;
    }

    for (size_t i = 0; i < n; i++) {
        struct json_object *item = json_object_array_get_idx(arr, (int)i);
        AVMarketInfo *m = &markets[i];
        if (!item || !json_object_is_type(item, json_type_object)) continue;
        m->market_type = av_json_get_string_dup(item, "market_type");
        m->region = av_json_get_string_dup(item, "region");
        m->primary_exchanges = av_json_get_string_dup(item, "primary_exchanges");
        m->local_open = av_json_get_string_dup(item, "local_open");
        m->local_close = av_json_get_string_dup(item, "local_close");
        m->current_status = av_json_get_string_dup(item, "current_status");
        m->notes = av_json_get_string_dup(item, "notes");
    }

    out->markets = markets;
    out->count = n;
    json_object_put(root);
    return AVOk;
}

/* Fetches and parses the global market open/close status (MARKET_STATUS).
 * On success, fills *out (caller must free with av_market_status_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_market_status(AVClient *client, AVMarketStatus *out) {
    if (!client || !out) return AVErrInvalidArg;
    out->endpoint = NULL; out->markets = NULL; out->count = 0;

    AVResponse resp;
    av_response_init(&resp);
    AVStatus st = av_query(client, "MARKET_STATUS", NULL, 0, &resp);
    if (st != AVOk) { av_response_free(&resp); return st; }
    if (resp.http_code != 200 || !resp.data) { av_response_free(&resp); return AVErrCurl; }
    st = av_parse_market_status(resp.data, out);
    av_response_free(&resp);
    return st;
}
