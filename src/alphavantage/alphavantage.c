#include "alphavantage.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

/* Default Alpha Vantage query endpoint. */
static const char *AV_DEFAULT_BASE_URL = "https://www.alphavantage.co/query";

typedef struct {
    char *data;
    size_t size;
} AVBuffer;

/* libcurl write callback that appends received bytes to an AVBuffer.
 * Grows the buffer with realloc, keeps it NUL-terminated, and updates its size.
 * Returns the number of bytes consumed (size*nmemb) on success, or 0 on
 * allocation failure to signal an error to libcurl. */
static size_t av_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    AVBuffer *buf = (AVBuffer *)userdata;

    char *next = (char *)realloc(buf->data, buf->size + total + 1);
    if (!next) {
        return 0; // Signals error to libcurl
    }

    buf->data = next;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

/* Initializes global libcurl state required by this library.
 * Returns AVOk on success, or AVErrCurlInit if curl_global_init fails. */
AVStatus av_global_init(void) {
    CURLcode rc = curl_global_init(CURL_GLOBAL_DEFAULT);
    return (rc == CURLE_OK) ? AVOk : AVErrCurlInit;
}

/* Releases global libcurl state initialized by av_global_init().
 * Returns nothing. */
void av_global_cleanup(void) {
    curl_global_cleanup();
}

/* Initializes an AVClient with the default base URL and an optional copied API key.
 * Returns AVOk on success, AVErrInvalidArg if client is NULL, or AVErrNomem on
 * allocation failure when copying api_key. */
AVStatus av_client_init(AVClient *client, const char *api_key) {
    if (!client) {
        return AVErrInvalidArg;
    }
    client->apikey = NULL;
    client->base_url = AV_DEFAULT_BASE_URL;

    if (api_key) {
        size_t len = strlen(api_key);
        client->apikey = (char *)malloc(len + 1);
        if (!client->apikey) {
            return AVErrNomem;
        }
        memcpy(client->apikey, api_key, len + 1);
    }
    return AVOk;
}

/* Releases resources owned by an AVClient and resets its fields.
 * Does not free the client struct itself. Returns nothing. */
void av_client_cleanup(AVClient *client) {
    if (!client) {
        return;
    }
    free(client->apikey);
    client->apikey = NULL;
    client->base_url = AV_DEFAULT_BASE_URL;
}

/* Initializes/clears an AVResponse for use with av_query().
 * Sets http_code/curl_code to 0 and data/size to NULL/0. Returns nothing. */
void av_response_init(AVResponse *resp) {
    if (!resp) {
        return;
    }
    resp->http_code = 0;
    resp->curl_code = 0;
    resp->data = NULL;
    resp->size = 0;
}

/* Frees memory owned by an AVResponse and resets its fields.
 * Safe to call on an empty or already-freed response. Returns nothing. */
void av_response_free(AVResponse *resp) {
    if (!resp) {
        return;
    }
    free(resp->data);
    resp->data = NULL;
    resp->size = 0;
}

/* Checks whether a key is present in a params array.
 * Returns 1 (true) if any entry has a matching key, otherwise 0 (false). */
static int av_has_param(const AVParam *params, size_t nparams, const char *key) {
    for (size_t i = 0; i < nparams; i++) {
        if (params[i].key && strcmp(params[i].key, key) == 0) {
            return 1;
        }
    }
    return 0;
}

/* URL-encodes a key/value pair and appends it as a query parameter to a CURLU.
 * Returns AVOk on success, AVErrInvalidArg if key or value is NULL, AVErrNomem
 * on allocation failure, or AVErrCurl if the URL cannot be updated. */
static AVStatus av_append_param(CURLU *urlh, CURL *curl, const char *key, const char *value) {
    if (!key || !value) {
        return AVErrInvalidArg;
    }
    char *ekey = curl_easy_escape(curl, key, 0);
    char *eval = curl_easy_escape(curl, value, 0);
    if (!ekey || !eval) {
        curl_free(ekey);
        curl_free(eval);
        return AVErrNomem;
    }

    size_t n = strlen(ekey) + 1 + strlen(eval) + 1;
    char *pair = (char *)malloc(n);
    if (!pair) {
        curl_free(ekey);
        curl_free(eval);
        return AVErrNomem;
    }
    /* format: key=value */
    /* snprintf not strictly necessary but safe */
    strcpy(pair, ekey);
    strcat(pair, "=");
    strcat(pair, eval);

    CURLUcode uc = curl_url_set(urlh, CURLUPART_QUERY, pair, CURLU_APPENDQUERY | CURLU_URLENCODE);
    free(pair);
    curl_free(ekey);
    curl_free(eval);
    return (uc == CURLUE_OK) ? AVOk : AVErrCurl;
}

/* Performs an HTTPS GET to the Alpha Vantage query endpoint for the given
 * function and parameters, storing the raw response body in resp.
 * Returns AVOk if the HTTP request completed (caller should inspect
 * resp->http_code and the body for API-level errors), AVErrInvalidArg if
 * required arguments are missing, AVErrCurlInit on handle/init failure, or
 * AVErrCurl on transport/URL errors (resp->curl_code holds the CURLcode). */
AVStatus av_query(AVClient *client,
                  const char *function,
                  const AVParam *params,
                  size_t nparams,
                  AVResponse *resp) {
    if (!client || !function || !resp) {
        return AVErrInvalidArg;
    }

    av_response_init(resp);

    CURL *curl = curl_easy_init();
    if (!curl) {
        return AVErrCurlInit;
    }

    CURLU *urlh = curl_url();
    AVBuffer buf = {0};
    AVStatus st = AVOk;

    if (!urlh) {
        st = AVErrCurlInit;
        goto done;
    }

    if (curl_url_set(urlh, CURLUPART_URL, client->base_url ? client->base_url : AV_DEFAULT_BASE_URL, 0) != CURLUE_OK) {
        st = AVErrCurl;
        goto done;
    }

    st = av_append_param(urlh, curl, "function", function);
    if (st != AVOk) goto done;

    for (size_t i = 0; i < nparams; i++) {
        if (!params[i].key) continue;
        st = av_append_param(urlh, curl, params[i].key, params[i].value ? params[i].value : "");
        if (st != AVOk) goto done;
    }

    /* Auto-append apikey from client if not explicitly provided. */
    if (client->apikey && !av_has_param(params, nparams, "apikey")) {
        st = av_append_param(urlh, curl, "apikey", client->apikey);
        if (st != AVOk) goto done;
    }

    curl_easy_setopt(curl, CURLOPT_CURLU, urlh);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, av_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "alphavantage-c/0.1");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); /* enable decompression if supported */
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    {
        CURLcode rc = curl_easy_perform(curl);
        resp->curl_code = (int)rc;
        if (rc != CURLE_OK) {
            st = AVErrCurl;
            /* keep any partial data only on success path; drop on transport error */
            free(buf.data);
            buf.data = NULL;
            buf.size = 0;
            goto done;
        }
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->http_code);
    resp->data = buf.data;
    resp->size = buf.size;
    buf.data = NULL; /* ownership transferred */

done:
    if (urlh) curl_url_cleanup(urlh);
    curl_easy_cleanup(curl);
    /* if failed before transfer, ensure no leak */
    if (st != AVOk && buf.data) {
        free(buf.data);
    }
    return st;
}

/* Returns a human-readable, NUL-terminated string naming the given AVStatus code.
 * Returns "AVErrUnknown" for unrecognized values. The pointer refers to static
 * storage and must not be freed. */
const char *av_status_str(AVStatus status) {
    switch (status) {
        case AVOk: return "AVOk";
        case AVErrInvalidArg: return "AVErrInvalidArg";
        case AVErrNomem: return "AVErrNomem";
        case AVErrCurlInit: return "AVErrCurlInit";
        case AVErrCurl: return "AVErrCurl";
        case AVErrBuffer: return "AVErrBuffer";
        default: return "AVErrUnknown";
    }
}
