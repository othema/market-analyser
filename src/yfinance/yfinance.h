#ifndef YFINANCE_H
#define YFINANCE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Status codes returned by Yahoo Finance interface functions.
typedef enum {
    YFOk = 0,
    YFErrInvalidArg,
    YFErrNomem,
    YFErrCurlInit,
    YFErrCurl,
    YFErrParse
} YFStatus;

// HTTP client configuration/state for Yahoo Finance requests.
typedef struct {
    const char *base_url;
    const char *user_agent;
} YFClient;

// Raw HTTP response produced by Yahoo Finance query functions.
// The caller owns data and must free it with yf_response_free.
typedef struct {
    long http_code;
    int curl_code;
    char *data;
    size_t size;
} YFResponse;

// Initializes global libcurl state required by this interface.
// Returns YFOk on success, or YFErrCurlInit on failure.
YFStatus yf_global_init(void);

// Releases global libcurl state initialized by yf_global_init.
// Returns nothing.
void yf_global_cleanup(void);

// Initializes a YFClient with default base URL and user agent.
// Returns YFOk on success, or YFErrInvalidArg for invalid input.
YFStatus yf_client_init(YFClient *client);

// Releases resources owned by a YFClient and resets its fields.
// Does not free the client struct itself. Returns nothing.
void yf_client_cleanup(YFClient *client);

// Initializes a YFResponse for use with query functions.
// Returns nothing.
void yf_response_init(YFResponse *response);

// Frees memory owned by a YFResponse and resets its fields.
// Safe to call on an empty or already-freed response. Returns nothing.
void yf_response_free(YFResponse *response);

// Fetches raw chart JSON for a symbol from Yahoo Finance.
// interval examples: "1d", "1wk", "1mo". range examples: "1mo", "6mo", "1y", "5y", "max".
// On transport success, stores the response body in response.
// Returns YFOk if the request completed, or a YFErr* status on failure.
YFStatus yf_fetch_chart(YFClient *client,
                        const char *symbol,
                        const char *interval,
                        const char *range,
                        YFResponse *response);

// Returns a human-readable string naming the given YFStatus code.
// The returned pointer refers to static storage and must not be freed.
const char *yf_status_str(YFStatus status);

// Includes parsed chart/time-series APIs for convenience.
#include "yf_types.h"
#include "yf_chart.h"

#ifdef __cplusplus
}
#endif

#endif // YFINANCE_H
