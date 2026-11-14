#ifndef AV_MARKET_H
#define AV_MARKET_H

#include "alphavantage.h"
#include "av_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parses a MARKET_STATUS JSON response body into an AVMarketStatus.
 * On success, allocates and fills *out (caller must free with av_market_status_free).
 * Returns AVOk on success, AVErrInvalidArg for bad inputs, AVErrNomem on
 * allocation failure, or AVErrCurl if the content is not a valid market status. */
AVStatus av_parse_market_status(const char *json_data, AVMarketStatus *out);

/* Fetches and parses the global market open/close status (MARKET_STATUS).
 * On success, fills *out (caller must free with av_market_status_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_market_status(AVClient *client, AVMarketStatus *out);

#ifdef __cplusplus
}
#endif

#endif /* AV_MARKET_H */
