#include "yfinance.h"
#include "yf_http.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

// Default Yahoo Finance chart endpoint base URL.
static const char *YF_DEFAULT_BASE_URL = "https://query1.finance.yahoo.com/v8/finance/chart/";

// Default user agent sent with Yahoo Finance requests.
static const char *YF_DEFAULT_USER_AGENT = "Mozilla/5.0";

// libcurl write callback that appends received bytes to a YFResponse.
static size_t yf_write_callback(char *ptr, size_t size, size_t count, void *userdata) {
    size_t total = size * count;
    YFResponse *response = (YFResponse *)userdata;

    char *next = (char *)realloc(response->data, response->size + total + 1);
    if (!next) {
        return 0;
    }

    response->data = next;
    memcpy(response->data + response->size, ptr, total);
    response->size += total;
    response->data[response->size] = '\0';
    return total;
}

char *yf_url_encode(const char *value) {
    if (!value) {
        return NULL;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    char *encoded = curl_easy_escape(curl, value, 0);
    curl_easy_cleanup(curl);
    return encoded;
}

char *yf_url_concat(const char *base, const char *query, const char *suffix) {
    if (!base || !query || !suffix) {
        return NULL;
    }

    size_t base_len = strlen(base);
    size_t query_len = strlen(query);
    size_t suffix_len = strlen(suffix);
    char *url = (char *)malloc(base_len + query_len + suffix_len + 1);
    if (!url) {
        return NULL;
    }

    memcpy(url, base, base_len);
    memcpy(url + base_len, query, query_len);
    memcpy(url + base_len + query_len, suffix, suffix_len + 1);
    return url;
}

YFStatus yf_http_get(const YFClient *client, const char *url,
                     long timeout_seconds, YFResponse *response) {
    if (!client || !url || !response) {
        return YFErrInvalidArg;
    }

    yf_response_init(response);

    CURL *curl = curl_easy_init();
    if (!curl) {
        return YFErrCurlInit;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, yf_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
                     client->user_agent ? client->user_agent : YF_DEFAULT_USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);

    CURLcode curl_result = curl_easy_perform(curl);
    response->curl_code = (int)curl_result;
    if (curl_result == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_code);
    }
    curl_easy_cleanup(curl);

    if (curl_result != CURLE_OK) {
        yf_response_free(response);
        return YFErrCurl;
    }
    return YFOk;
}

// Initializes global libcurl state required by this interface.
YFStatus yf_global_init(void) {
    CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (result != CURLE_OK) {
        return YFErrCurlInit;
    }
    return YFOk;
}

// Releases global libcurl state initialized by yf_global_init.
void yf_global_cleanup(void) {
    curl_global_cleanup();
}

// Initializes a YFClient with default base URL and user agent.
YFStatus yf_client_init(YFClient *client) {
    if (!client) {
        return YFErrInvalidArg;
    }

    client->base_url = YF_DEFAULT_BASE_URL;
    client->user_agent = YF_DEFAULT_USER_AGENT;
    return YFOk;
}

// Releases resources owned by a YFClient and resets its fields.
// Does not free the client struct itself.
void yf_client_cleanup(YFClient *client) {
    if (!client) {
        return;
    }

    client->base_url = YF_DEFAULT_BASE_URL;
    client->user_agent = YF_DEFAULT_USER_AGENT;
}

// Initializes a YFResponse for use with query functions.
void yf_response_init(YFResponse *response) {
    if (!response) {
        return;
    }

    response->http_code = 0;
    response->curl_code = 0;
    response->data = NULL;
    response->size = 0;
}

// Frees memory owned by a YFResponse and resets its fields.
// Safe to call on an empty or already-freed response.
void yf_response_free(YFResponse *response) {
    if (!response) {
        return;
    }

    free(response->data);
    response->data = NULL;
    response->size = 0;
    response->http_code = 0;
    response->curl_code = 0;
}

// Fetches raw chart JSON for a symbol from Yahoo Finance.
YFStatus yf_fetch_chart(YFClient *client,
                        const char *symbol,
                        const char *interval,
                        const char *range,
                        YFResponse *response) {
    if (!client || !symbol || !interval || !range || !response) {
        return YFErrInvalidArg;
    }

    char *encoded_symbol = yf_url_encode(symbol);
    char *encoded_interval = yf_url_encode(interval);
    char *encoded_range = yf_url_encode(range);
    if (!encoded_symbol || !encoded_interval || !encoded_range) {
        curl_free(encoded_symbol);
        curl_free(encoded_interval);
        curl_free(encoded_range);
        return YFErrNomem;
    }

    char suffix[128];
    int written = snprintf(suffix, sizeof(suffix), "?interval=%s&range=%s",
                           encoded_interval, encoded_range);
    curl_free(encoded_interval);
    curl_free(encoded_range);
    if (written < 0 || (size_t)written >= sizeof(suffix)) {
        curl_free(encoded_symbol);
        return YFErrNomem;
    }

    const char *base = client->base_url ? client->base_url : YF_DEFAULT_BASE_URL;
    char *url = yf_url_concat(base, encoded_symbol, suffix);
    curl_free(encoded_symbol);
    if (!url) {
        return YFErrNomem;
    }

    YFStatus status = yf_http_get(client, url, 30L, response);
    free(url);
    return status;
}

// The returned pointer refers to static storage and must not be freed.
const char *yf_status_str(YFStatus status) {
    switch (status) {
        case YFOk:
            return "YFOk";
        case YFErrInvalidArg:
            return "YFErrInvalidArg";
        case YFErrNomem:
            return "YFErrNomem";
        case YFErrCurlInit:
            return "YFErrCurlInit";
        case YFErrCurl:
            return "YFErrCurl";
        case YFErrParse:
            return "YFErrParse";
        default:
            return "YFErrUnknown";
    }
}
