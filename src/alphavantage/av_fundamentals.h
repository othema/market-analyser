#ifndef AV_FUNDAMENTALS_H
#define AV_FUNDAMENTALS_H

#include "alphavantage.h"
#include "av_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Parses an OVERVIEW JSON response body into an AVCompanyOverview.
// On success, fills *out (caller must free with av_company_overview_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_parse_company_overview(const char *json_data, AVCompanyOverview *out);

// Fetches and parses company overview data (OVERVIEW) for a symbol.
// On success, fills *out (caller must free with av_company_overview_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_company_overview(AVClient *client, const char *symbol, AVCompanyOverview *out);

// Parses a COMPANY_LOGO JSON response body into an AVCompanyLogo.
// On success, fills *out (caller must free with av_company_logo_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_parse_company_logo(const char *json_data, AVCompanyLogo *out);

// Fetches and parses company logo URLs (COMPANY_LOGO) for a symbol.
// On success, fills *out (caller must free with av_company_logo_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_company_logo(AVClient *client, const char *symbol, AVCompanyLogo *out);

#ifdef __cplusplus
}
#endif

#endif // AV_FUNDAMENTALS_H
