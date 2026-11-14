#ifndef ALPHAVANTAGE_H
#define ALPHAVANTAGE_H

#include <stddef.h>

#include "av_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal C client for the Alpha Vantage HTTP API.
 *
 * This library centers on a single "master" query function (av_query) that
 * performs an HTTPS GET against https://www.alphavantage.co/query with a
 * caller-supplied function name and query parameters. The same function can be
 * used for all endpoints (time series, SYMBOL_SEARCH, MARKET_STATUS, OVERVIEW,
 * COMPANY_LOGO, etc.).
 *
 * Dependencies: libcurl (linked by the consumer).
 */

/* Result codes returned by this library. */
typedef enum {
    AVOk = 0,
    AVErrInvalidArg,
    AVErrNomem,
    AVErrCurlInit,
    AVErrCurl,        /* HTTP/transport error; see curl_code / message */
    AVErrBuffer       /* response accumulation failed */
} AVStatus;

/* A single query parameter (key=value). Values are URL-encoded by av_query. */
typedef struct {
    const char *key;
    const char *value;
} AVParam;

/* Client configuration/handle. Initialize with av_client_init(). */
typedef struct {
    char *apikey;       /* owned copy; may be NULL if you pass apikey as a param */
    const char *base_url; /* defaults to https://www.alphavantage.co/query */
} AVClient;

/* Response buffer produced by av_query(). Free with av_response_free(). */
typedef struct {
    long http_code;     /* HTTP status code (e.g. 200), or 0 if request failed */
    int curl_code;      /* CURLcode as int (0 == CURLE_OK) */
    char *data;         /* response body (NUL-terminated); NULL on failure */
    size_t size;        /* length of data in bytes, excluding NUL terminator */
} AVResponse;

/* Initializes global libcurl state required by this library.
 * Call once before making queries (e.g. at program start).
 * Returns AVOk on success, or AVErrCurlInit if curl_global_init fails. */
AVStatus av_global_init(void);

/* Releases global libcurl state initialized by av_global_init().
 * Call once at program shutdown. Returns nothing. */
void av_global_cleanup(void);

/* Initializes an AVClient with the default base URL and an optional API key.
 * If api_key is non-NULL it is copied and automatically appended by av_query()
 * unless an "apikey" parameter is supplied explicitly; api_key may be NULL.
 * Returns AVOk on success, AVErrInvalidArg if client is NULL, or AVErrNomem if
 * the API key copy cannot be allocated. */
AVStatus av_client_init(AVClient *client, const char *api_key);

/* Releases resources owned by an AVClient (e.g. the copied API key) and resets
 * its fields. Does not free the client struct itself. Returns nothing. */
void av_client_cleanup(AVClient *client);

/* Initializes/clears an AVResponse so it is safe to pass to av_query().
 * Sets http_code/curl_code to 0 and data/size to NULL/0. Returns nothing. */
void av_response_init(AVResponse *resp);

/* Frees memory owned by an AVResponse (its data buffer) and resets its fields.
 * Safe to call on an empty or already-freed response. Returns nothing. */
void av_response_free(AVResponse *resp);

/*
 * Master query function.
 *
 * Performs: GET <client->base_url>?function=<function>&<params...>[&apikey=...]
 *
 * Parameters:
 *   client   - initialized AVClient (must not be NULL)
 *   function - required AV function name, e.g. "TIME_SERIES_DAILY",
 *              "TIME_SERIES_DAILY_ADJUSTED", "TIME_SERIES_WEEKLY",
 *              "TIME_SERIES_WEEKLY_ADJUSTED", "TIME_SERIES_MONTHLY",
 *              "TIME_SERIES_MONTHLY_ADJUSTED", "SYMBOL_SEARCH",
 *              "MARKET_STATUS", "OVERVIEW", "COMPANY_LOGO"
 *   params   - array of additional key/value pairs (may be NULL if nparams==0)
 *   nparams  - number of elements in params
 *   resp     - output response (must not be NULL); caller frees with
 *              av_response_free()
 *
 * Returns AVOk if the HTTP request completed (check resp->http_code and the
 * response body for API-level errors/rate-limit notes). Returns an AVErr*
 * code for local/transport failures (see resp->curl_code).
 *
 * Examples:
 *   AVParam p1[] = { {"symbol", "IBM"}, {"datatype", "csv"} };
 *   av_query(&client, "TIME_SERIES_DAILY", p1, 2, &resp);
 *
 *   AVParam p2[] = { {"keywords", "microsoft"} };
 *   av_query(&client, "SYMBOL_SEARCH", p2, 1, &resp);
 *
 *   av_query(&client, "MARKET_STATUS", NULL, 0, &resp);
 *
 *   AVParam p3[] = { {"symbol", "IBM"} };
 *   av_query(&client, "OVERVIEW", p3, 1, &resp);
 *   av_query(&client, "COMPANY_LOGO", p3, 1, &resp);
 */
AVStatus av_query(AVClient *client,
                  const char *function,
                  const AVParam *params,
                  size_t nparams,
                  AVResponse *resp);

/* Returns a human-readable, NUL-terminated string naming the given AVStatus
 * code (e.g. "AVOk", "AVErrCurl"). Returns "AVErrUnknown" for unrecognized
 * values. The returned pointer refers to static storage and must not be freed. */
const char *av_status_str(AVStatus status);

/* High-level, parsed endpoint APIs. */
#include "av_time_series.h"
#include "av_search.h"
#include "av_market.h"
#include "av_fundamentals.h"

#ifdef __cplusplus
}
#endif

#endif /* ALPHAVANTAGE_H */
