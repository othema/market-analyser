#include "yfinance.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

// Default Yahoo Finance chart endpoint base URL.
static const char *YF_DEFAULT_BASE_URL = "https://query1.finance.yahoo.com/v8/finance/chart/";

// Default user agent sent with Yahoo Finance requests.
static const char *YF_DEFAULT_USER_AGENT = "Mozilla/5.0";

// Dynamic byte buffer used to accumulate HTTP response data.
typedef struct {
    char *data;
    size_t size;
} YFBuffer;

// libcurl write callback that appends received bytes to a YFBuffer.
// Keeps the buffer NUL-terminated and grows it as needed.
// Returns the number of bytes consumed on success, or 0 on allocation failure.
static size_t yf_write_callback(char *ptr, size_t size, size_t count, void *userdata) {
    size_t total = size * count;
    YFBuffer *buffer = (YFBuffer *)userdata;

    char *next = (char *)realloc(buffer->data, buffer->size + total + 1);
    if (!next) {
        return 0;
    }

    buffer->data = next;
    memcpy(buffer->data + buffer->size, ptr, total);
    buffer->size += total;
    buffer->data[buffer->size] = '\0';
    return total;
}

// Initializes global libcurl state required by this interface.
// Returns YFOk on success, or YFErrCurlInit on failure.
YFStatus yf_global_init(void) {
    CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (result != CURLE_OK) {
        return YFErrCurlInit;
    }
    return YFOk;
}

// Releases global libcurl state initialized by yf_global_init.
// Returns nothing.
void yf_global_cleanup(void) {
    curl_global_cleanup();
}

// Initializes a YFClient with default base URL and user agent.
// Returns YFOk on success, or YFErrInvalidArg for invalid input.
YFStatus yf_client_init(YFClient *client) {
    if (!client) {
        return YFErrInvalidArg;
    }

    client->base_url = YF_DEFAULT_BASE_URL;
    client->user_agent = YF_DEFAULT_USER_AGENT;
    return YFOk;
}

// Releases resources owned by a YFClient and resets its fields.
// Does not free the client struct itself. Returns nothing.
void yf_client_cleanup(YFClient *client) {
    if (!client) {
        return;
    }

    client->base_url = YF_DEFAULT_BASE_URL;
    client->user_agent = YF_DEFAULT_USER_AGENT;
}

// Initializes a YFResponse for use with query functions.
// Returns nothing.
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
// Safe to call on an empty or already-freed response. Returns nothing.
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

// Builds a Yahoo Finance chart request URL for a symbol, interval, and range.
// Returns a heap-allocated URL string on success, or NULL on failure.
// The caller is responsible for freeing the returned string.
static char *yf_build_chart_url(CURL *curl, const char *base_url, const char *symbol, const char *interval, const char *range) {
    if (!curl || !base_url || !symbol || !interval || !range) {
        return NULL;
    }

    char *encoded_symbol = curl_easy_escape(curl, symbol, 0);
    char *encoded_interval = curl_easy_escape(curl, interval, 0);
    char *encoded_range = curl_easy_escape(curl, range, 0);
    if (!encoded_symbol || !encoded_interval || !encoded_range) {
        curl_free(encoded_symbol);
        curl_free(encoded_interval);
        curl_free(encoded_range);
        return NULL;
    }

    const char *prefix = "?interval=";
    const char *mid = "&range=";
    size_t url_size = strlen(base_url) + strlen(encoded_symbol) +
                      strlen(prefix) + strlen(encoded_interval) +
                      strlen(mid) + strlen(encoded_range) + 1;

    char *url = (char *)malloc(url_size);
    if (!url) {
        curl_free(encoded_symbol);
        curl_free(encoded_interval);
        curl_free(encoded_range);
        return NULL;
    }

    strcpy(url, base_url);
    strcat(url, encoded_symbol);
    strcat(url, prefix);
    strcat(url, encoded_interval);
    strcat(url, mid);
    strcat(url, encoded_range);

    curl_free(encoded_symbol);
    curl_free(encoded_interval);
    curl_free(encoded_range);
    return url;
}

// Fetches raw chart JSON for a symbol from Yahoo Finance.
// interval examples: "1d", "1wk", "1mo". range examples: "1mo", "6mo", "1y", "5y", "max".
// On transport success, stores the response body in response.
// Returns YFOk if the request completed, or a YFErr* status on failure.
YFStatus yf_fetch_chart(YFClient *client,
                        const char *symbol,
                        const char *interval,
                        const char *range,
                        YFResponse *response) {
    if (!client || !symbol || !interval || !range || !response) {
        return YFErrInvalidArg;
    }

    yf_response_init(response);

    CURL *curl = curl_easy_init();
    if (!curl) {
        return YFErrCurlInit;
    }

    YFBuffer buffer = {0};
    YFStatus status = YFOk;
    char *url = yf_build_chart_url(curl,
                                   client->base_url ? client->base_url : YF_DEFAULT_BASE_URL,
                                   symbol,
                                   interval,
                                   range);
    if (!url) {
        curl_easy_cleanup(curl);
        return YFErrNomem;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, yf_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, client->user_agent ? client->user_agent : YF_DEFAULT_USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    {
        CURLcode curl_result = curl_easy_perform(curl);
        response->curl_code = (int)curl_result;
        if (curl_result != CURLE_OK) {
            status = YFErrCurl;
            free(buffer.data);
            buffer.data = NULL;
            buffer.size = 0;
            goto cleanup;
        }
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_code);
    response->data = buffer.data;
    response->size = buffer.size;
    buffer.data = NULL;

cleanup:
    free(url);
    curl_easy_cleanup(curl);
    if (status != YFOk && buffer.data) {
        free(buffer.data);
    }
    return status;
}

// Returns a human-readable string naming the given YFStatus code.
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
