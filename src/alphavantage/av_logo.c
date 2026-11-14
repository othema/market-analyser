#include "av_fundamentals.h"
#include "av_utils.h"

#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

// Frees all memory owned by an AVCompanyLogo and resets its fields.
// Safe to call on an empty or already-freed logo. Returns nothing.
void av_company_logo_free(AVCompanyLogo *logo) {
    if (!logo) {
        return;
    }

    free(logo->symbol);
    free(logo->logo_url_png);
    free(logo->logo_url_svg);
    memset(logo, 0, sizeof(*logo));
}

// Parses a COMPANY_LOGO JSON response body into an AVCompanyLogo.
// On success, fills *out (caller must free with av_company_logo_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_parse_company_logo(const char *json_data, AVCompanyLogo *out) {
    if (!json_data || !out) {
        return AVErrInvalidArg;
    }

    memset(out, 0, sizeof(*out));
    if (av_response_is_error(json_data)) {
        return AVErrCurl;
    }

    struct json_object *root = json_tokener_parse(json_data);
    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) {
            json_object_put(root);
        }
        return AVErrCurl;
    }

    // Expect at least symbol or one logo URL field.
    if (!json_object_object_get_ex(root, "symbol", NULL) &&
        !json_object_object_get_ex(root, "logo_url_png", NULL) &&
        !json_object_object_get_ex(root, "logo_url_svg", NULL)) {
        json_object_put(root);
        return AVErrCurl;
    }

    out->symbol = av_json_get_string_dup(root, "symbol");
    out->logo_url_png = av_json_get_string_dup(root, "logo_url_png");
    out->logo_url_svg = av_json_get_string_dup(root, "logo_url_svg");

    json_object_put(root);
    return AVOk;
}

// Fetches and parses company logo URLs (COMPANY_LOGO) for a symbol.
// On success, fills *out (caller must free with av_company_logo_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_company_logo(AVClient *client, const char *symbol, AVCompanyLogo *out) {
    if (!client || !symbol || !out) {
        return AVErrInvalidArg;
    }

    memset(out, 0, sizeof(*out));
    AVParam params[] = {{"symbol", symbol}};
    AVResponse response;
    av_response_init(&response);

    AVStatus status = av_query(client, "COMPANY_LOGO", params, 1, &response);
    if (status != AVOk) {
        av_response_free(&response);
        return status;
    }
    if (response.http_code != 200 || !response.data) {
        av_response_free(&response);
        return AVErrCurl;
    }

    status = av_parse_company_logo(response.data, out);
    av_response_free(&response);
    return status;
}
