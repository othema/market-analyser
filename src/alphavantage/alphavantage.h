#ifndef ALPHAVANTAGE_H
#define ALPHAVANTAGE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Status codes returned by Alpha Vantage library functions.
typedef enum {
    AVOk = 0,
    AVErrInvalidArg,
    AVErrNomem,
    AVErrCurlInit,
    AVErrCurl,
    AVErrBuffer
} AVStatus;

// HTTP client configuration/state for Alpha Vantage requests.
// The client copies and owns apikey when provided.
typedef struct {
    char *apikey;
    const char *base_url;
} AVClient;

// Key/value query parameter passed to av_query.
typedef struct {
    const char *key;
    const char *value;
} AVParam;

// Raw HTTP response produced by av_query.
// The caller owns resp->data and must free it with av_response_free.
typedef struct {
    long http_code;
    int curl_code;
    char *data;
    size_t size;
} AVResponse;

// Initializes global libcurl state required by this library.
// Returns AVOk on success, or AVErrCurlInit on failure.
AVStatus av_global_init(void);

// Releases global libcurl state initialized by av_global_init.
// Returns nothing.
void av_global_cleanup(void);

// Initializes an AVClient with the default base URL and an optional copied API key.
// Returns AVOk on success, AVErrInvalidArg for invalid input, or AVErrNomem on allocation failure.
AVStatus av_client_init(AVClient *client, const char *api_key);

// Releases resources owned by an AVClient and resets its fields.
// Does not free the client struct itself. Returns nothing.
void av_client_cleanup(AVClient *client);

// Initializes an AVResponse for use with av_query.
// Returns nothing.
void av_response_init(AVResponse *resp);

// Frees memory owned by an AVResponse and resets its fields.
// Safe to call on an empty or already-freed response. Returns nothing.
void av_response_free(AVResponse *resp);

// Performs an HTTPS GET request to the Alpha Vantage query endpoint.
// Stores the raw response body in resp on transport success.
// Returns AVOk if the request completed (check resp->http_code/body for API errors),
// or an AVErr* status on argument/transport/initialization failure.
AVStatus av_query(AVClient *client,
                  const char *function,
                  const AVParam *params,
                  size_t nparams,
                  AVResponse *resp);

// Returns a human-readable string naming the given AVStatus code.
// The returned pointer refers to static storage and must not be freed.
const char *av_status_str(AVStatus status);

// Includes parsed endpoint/type APIs for convenience.
#include "av_types.h"
#include "av_time_series.h"
#include "av_search.h"
#include "av_market.h"
#include "av_fundamentals.h"

#ifdef __cplusplus
}
#endif

#endif // ALPHAVANTAGE_H
