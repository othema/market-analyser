#include "av_market.h"
#include "av_utils.h"

#include <json-c/json.h>
#include <stdlib.h>

// Frees all memory owned by an AVMarketStatus and resets its fields.
// Safe to call on an empty or already-freed status. Returns nothing.
void av_market_status_free(AVMarketStatus *status) {
    if (!status) {
        return;
    }

    free(status->endpoint);
    for (size_t i = 0; i < status->count; i++) {
        AVMarketInfo *market = &status->markets[i];
        free(market->market_type);
        free(market->region);
        free(market->primary_exchanges);
        free(market->local_open);
        free(market->local_close);
        free(market->current_status);
        free(market->notes);
    }

    free(status->markets);
    status->endpoint = NULL;
    status->markets = NULL;
    status->count = 0;
}

// Parses a MARKET_STATUS JSON response body into an AVMarketStatus.
// On success, fills *out (caller must free with av_market_status_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_parse_market_status(const char *json_data, AVMarketStatus *out) {
    if (!json_data || !out) {
        return AVErrInvalidArg;
    }

    out->endpoint = NULL;
    out->markets = NULL;
    out->count = 0;
    if (av_response_is_error(json_data)) {
        return AVErrCurl;
    }

    struct json_object *root = json_tokener_parse(json_data);
    if (!root) {
        return AVErrCurl;
    }

    struct json_object *markets_array = NULL;
    if (!json_object_object_get_ex(root, "markets", &markets_array) ||
        !json_object_is_type(markets_array, json_type_array)) {
        json_object_put(root);
        return AVErrCurl;
    }

    out->endpoint = av_json_get_string_dup(root, "endpoint");

    size_t count = (size_t)json_object_array_length(markets_array);
    if (count == 0) {
        json_object_put(root);
        return AVOk;
    }

    AVMarketInfo *markets = (AVMarketInfo *)calloc(count, sizeof(AVMarketInfo));
    if (!markets) {
        json_object_put(root);
        av_market_status_free(out);
        return AVErrNomem;
    }

    for (size_t i = 0; i < count; i++) {
        struct json_object *item = json_object_array_get_idx(markets_array, (int)i);
        if (!item || !json_object_is_type(item, json_type_object)) {
            continue;
        }

        AVMarketInfo *market = &markets[i];
        market->market_type = av_json_get_string_dup(item, "market_type");
        market->region = av_json_get_string_dup(item, "region");
        market->primary_exchanges = av_json_get_string_dup(item, "primary_exchanges");
        market->local_open = av_json_get_string_dup(item, "local_open");
        market->local_close = av_json_get_string_dup(item, "local_close");
        market->current_status = av_json_get_string_dup(item, "current_status");
        market->notes = av_json_get_string_dup(item, "notes");
    }

    out->markets = markets;
    out->count = count;
    json_object_put(root);
    return AVOk;
}

// Fetches and parses the global market open/close status (MARKET_STATUS).
// On success, fills *out (caller must free with av_market_status_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_market_status(AVClient *client, AVMarketStatus *out) {
    if (!client || !out) {
        return AVErrInvalidArg;
    }

    out->endpoint = NULL;
    out->markets = NULL;
    out->count = 0;

    AVResponse response;
    av_response_init(&response);
    AVStatus status = av_query(client, "MARKET_STATUS", NULL, 0, &response);
    if (status != AVOk) {
        av_response_free(&response);
        return status;
    }
    if (response.http_code != 200 || !response.data) {
        av_response_free(&response);
        return AVErrCurl;
    }

    status = av_parse_market_status(response.data, out);
    av_response_free(&response);
    return status;
}
