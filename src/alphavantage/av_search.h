#ifndef AV_SEARCH_H
#define AV_SEARCH_H

#include "alphavantage.h"
#include "av_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parses a SYMBOL_SEARCH JSON response body into an AVSearchResult.
// On success, fills *out (caller must free with av_search_result_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_parse_search_result(const char *json_data, AVSearchResult *out);

// Fetches and parses SYMBOL_SEARCH results for the given keywords.
// On success, fills *out (caller must free with av_search_result_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_search_symbol(AVClient *client, const char *keywords, AVSearchResult *out);

#ifdef __cplusplus
}
#endif

#endif // AV_SEARCH_H
