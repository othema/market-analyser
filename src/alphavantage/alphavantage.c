#include "alphavantage.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

// Default Alpha Vantage query endpoint URL.
static const char *AV_DEFAULT_BASE_URL = "https://www.alphavantage.co/query";

// Dynamic byte buffer used to accumulate HTTP response data.
typedef struct {
    char *data;
    size_t size;
} AVBuffer;

// libcurl write callback that appends received bytes to an AVBuffer.
// Keeps the buffer NUL-terminated and grows it as needed.
// Returns the number of bytes consumed on success, or 0 on allocation failure.
static size_t av_write_callback(char *ptr, size_t size, size_t count, void *userdata) {
    size_t total = size * count;
    AVBuffer *buffer = (AVBuffer *)userdata;

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

// Initializes global libcurl state required by this library.
// Returns AVOk on success, or AVErrCurlInit on failure.
AVStatus av_global_init(void) {
    CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (result != CURLE_OK) {
        return AVErrCurlInit;
    }
    return AVOk;
}

// Releases global libcurl state initialized by av_global_init.
// Returns nothing.
void av_global_cleanup(void) {
    curl_global_cleanup();
}

// Initializes an AVClient with the default base URL and an optional copied API key.
// Returns AVOk on success, AVErrInvalidArg for invalid input, or AVErrNomem on allocation failure.
AVStatus av_client_init(AVClient *client, const char *api_key) {
    if (!client) {
        return AVErrInvalidArg;
    }

    client->apikey = NULL;
    client->base_url = AV_DEFAULT_BASE_URL;

    if (api_key) {
        size_t length = strlen(api_key);
        client->apikey = (char *)malloc(length + 1);
        if (!client->apikey) {
            return AVErrNomem;
        }
        memcpy(client->apikey, api_key, length + 1);
    }

    return AVOk;
}

// Releases resources owned by an AVClient and resets its fields.
// Does not free the client struct itself. Returns nothing.
void av_client_cleanup(AVClient *client) {
    if (!client) {
        return;
    }

    free(client->apikey);
    client->apikey = NULL;
    client->base_url = AV_DEFAULT_BASE_URL;
}

// Initializes an AVResponse for use with av_query.
// Returns nothing.
void av_response_init(AVResponse *response) {
    if (!response) {
        return;
    }

    response->http_code = 0;
    response->curl_code = 0;
    response->data = NULL;
    response->size = 0;
}

// Frees memory owned by an AVResponse and resets its fields.
// Safe to call on an empty or already-freed response. Returns nothing.
void av_response_free(AVResponse *response) {
    if (!response) {
        return;
    }

    free(response->data);
    response->data = NULL;
    response->size = 0;
    response->http_code = 0;
    response->curl_code = 0;
}

// Checks whether a key is present in a query parameter array.
// Returns 1 if the key exists, otherwise 0.
static int av_has_param(const AVParam *params, size_t param_count, const char *key) {
    if (!key) {
        return 0;
    }

    for (size_t i = 0; i < param_count; i++) {
        if (params[i].key && strcmp(params[i].key, key) == 0) {
            return 1;
        }
    }
    return 0;
}

// Appends a key/value query parameter to a CURLU URL handle.
// Returns AVOk on success, or an AVErr* status on failure.
static AVStatus av_append_query_param(CURLU *url_handle, CURL *curl, const char *key, const char *value) {
    if (!key || !value) {
        return AVErrInvalidArg;
    }

    char *encoded_key = curl_easy_escape(curl, key, 0);
    char *encoded_value = curl_easy_escape(curl, value, 0);
    if (!encoded_key || !encoded_value) {
        curl_free(encoded_key);
        curl_free(encoded_value);
        return AVErrNomem;
    }

    size_t pair_size = strlen(encoded_key) + 1 + strlen(encoded_value) + 1;
    char *pair = (char *)malloc(pair_size);
    if (!pair) {
        curl_free(encoded_key);
        curl_free(encoded_value);
        return AVErrNomem;
    }

    // Build "key=value" for appending to the query string.
    strcpy(pair, encoded_key);
    strcat(pair, "=");
    strcat(pair, encoded_value);

    CURLUcode url_result = curl_url_set(url_handle, CURLUPART_QUERY, pair, CURLU_APPENDQUERY | CURLU_URLENCODE);

    free(pair);
    curl_free(encoded_key);
    curl_free(encoded_value);

    if (url_result != CURLUE_OK) {
        return AVErrCurl;
    }
    return AVOk;
}

// Performs an HTTPS GET request to the Alpha Vantage query endpoint.
// Stores the raw response body in response on transport success.
// Returns AVOk if the request completed (check response->http_code/body for API errors),
// or an AVErr* status on argument/transport/initialization failure.
AVStatus av_query(AVClient *client,
                  const char *function,
                  const AVParam *params,
                  size_t param_count,
                  AVResponse *response) {
    if (!client || !function || !response) {
        return AVErrInvalidArg;
    }

    av_response_init(response);

    CURL *curl = curl_easy_init();
    if (!curl) {
        return AVErrCurlInit;
    }

    CURLU *url_handle = curl_url();
    AVBuffer buffer = {0};
    AVStatus status = AVOk;

    if (!url_handle) {
        status = AVErrCurlInit;
        goto cleanup;
    }

    if (curl_url_set(url_handle, CURLUPART_URL, client->base_url ? client->base_url : AV_DEFAULT_BASE_URL, 0) != CURLUE_OK) {
        status = AVErrCurl;
        goto cleanup;
    }

    status = av_append_query_param(url_handle, curl, "function", function);
    if (status != AVOk) {
        goto cleanup;
    }

    for (size_t i = 0; i < param_count; i++) {
        if (!params[i].key) {
            continue;
        }
        status = av_append_query_param(url_handle, curl, params[i].key, params[i].value ? params[i].value : "");
        if (status != AVOk) {
            goto cleanup;
        }
    }

    // Automatically append apikey from client if not explicitly provided.
    if (client->apikey && !av_has_param(params, param_count, "apikey")) {
        status = av_append_query_param(url_handle, curl, "apikey", client->apikey);
        if (status != AVOk) {
            goto cleanup;
        }
    }

    curl_easy_setopt(curl, CURLOPT_CURLU, url_handle);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, av_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "alphavantage-c/0.1");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    {
        CURLcode curl_result = curl_easy_perform(curl);
        response->curl_code = (int)curl_result;
        if (curl_result != CURLE_OK) {
            status = AVErrCurl;
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
    if (url_handle) {
        curl_url_cleanup(url_handle);
    }
    curl_easy_cleanup(curl);
    if (status != AVOk && buffer.data) {
        free(buffer.data);
    }
    return status;
}

// Returns a human-readable string naming the given AVStatus code.
// The returned pointer refers to static storage and must not be freed.
const char *av_status_str(AVStatus status) {
    switch (status) {
        case AVOk:
            return "AVOk";
        case AVErrInvalidArg:
            return "AVErrInvalidArg";
        case AVErrNomem:
            return "AVErrNomem";
        case AVErrCurlInit:
            return "AVErrCurlInit";
        case AVErrCurl:
            return "AVErrCurl";
        case AVErrBuffer:
            return "AVErrBuffer";
        default:
            return "AVErrUnknown";
    }
}
