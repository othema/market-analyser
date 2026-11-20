#ifndef YF_HTTP_H
#define YF_HTTP_H

#include "yfinance/yfinance.h"

#ifdef __cplusplus
extern "C" {
#endif

// Internal helpers shared by the Yahoo Finance client modules.
// These are not part of the public yfinance API.

// Percent-encodes a value for use in a URL query.
// Returns a curl-allocated string that the caller must release with curl_free.
char *yf_url_encode(const char *value);

// Joins a base URL, an encoded query and a suffix into a heap-allocated URL.
// Returns NULL on invalid input or allocation failure; caller must free.
char *yf_url_concat(const char *base, const char *query, const char *suffix);

// Performs a GET request into response using the client's user agent.
// Transport failures return YFErrCurl; the HTTP status is left in
// response->http_code for the caller to inspect.
YFStatus yf_http_get(const YFClient *client, const char *url,
                     long timeout_seconds, YFResponse *response);

#ifdef __cplusplus
}
#endif

#endif // YF_HTTP_H
