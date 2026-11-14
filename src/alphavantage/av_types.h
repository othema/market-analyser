#ifndef AV_TYPES_H
#define AV_TYPES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A single data point in a time series (daily/weekly/monthly, raw or adjusted). */
typedef struct {
    char *date;                /* Timestamp/date string (e.g. "2024-01-02"), heap-allocated. */
    double open;
    double high;
    double low;
    double close;
    double adjusted_close;     /* Set for adjusted series; 0.0 if not applicable. */
    long long volume;
    double dividend_amount;    /* Set for adjusted series where provided; 0.0 otherwise. */
    double split_coefficient;  /* Set for daily adjusted where provided; 0.0 otherwise. */
    int has_adjusted_close;
    int has_dividend_amount;
    int has_split_coefficient;
} AVTimeSeriesPoint;

/* Parsed time series result. Points are in the same order as returned by the API
 * (typically most recent first for Alpha Vantage CSV/JSON time series). */
typedef struct {
    AVTimeSeriesPoint *points;
    size_t count;
} AVTimeSeries;

/* A single match returned by SYMBOL_SEARCH. All strings are heap-allocated (or NULL). */
typedef struct {
    char *symbol;
    char *name;
    char *type;
    char *region;
    char *market_open;
    char *market_close;
    char *timezone;
    char *currency;
    char *match_score;
} AVSearchMatch;

/* Parsed SYMBOL_SEARCH result. */
typedef struct {
    AVSearchMatch *matches;
    size_t count;
} AVSearchResult;

/* Market status entry returned by MARKET_STATUS. All strings are heap-allocated (or NULL). */
typedef struct {
    char *market_type;
    char *region;
    char *primary_exchanges;
    char *local_open;
    char *local_close;
    char *current_status;
    char *notes;
} AVMarketInfo;

/* Parsed MARKET_STATUS result. */
typedef struct {
    char *endpoint;            /* Endpoint label if provided; may be NULL. */
    AVMarketInfo *markets;
    size_t count;
} AVMarketStatus;

/* Company overview data returned by OVERVIEW. All string fields are heap-allocated
 * (or NULL if missing). This includes commonly used fields; unmapped/additional
 * fields from the API are not stored individually. */
typedef struct {
    char *symbol;
    char *asset_type;
    char *name;
    char *description;
    char *cik;
    char *exchange;
    char *currency;
    char *country;
    char *sector;
    char *industry;
    char *address;
    char *fiscal_year_end;
    char *latest_quarter;
    char *market_capitalization;
    char *ebitda;
    char *pe_ratio;
    char *peg_ratio;
    char *book_value;
    char *dividend_per_share;
    char *dividend_yield;
    char *eps;
    char *revenue_per_share_ttm;
    char *profit_margin;
    char *operating_margin_ttm;
    char *return_on_assets_ttm;
    char *return_on_equity_ttm;
    char *revenue_ttm;
    char *gross_profit_ttm;
    char *diluted_eps_ttm;
    char *quarterly_earnings_growth_yoy;
    char *quarterly_revenue_growth_yoy;
    char *analyst_target_price;
    char *trailing_pe;
    char *forward_pe;
    char *price_to_sales_ratio_ttm;
    char *price_to_book_ratio;
    char *ev_to_revenue;
    char *ev_to_ebitda;
    char *beta;
    char *week_52_high;
    char *week_52_low;
    char *day_50_moving_average;
    char *day_200_moving_average;
    char *shares_outstanding;
    char *dividend_date;
    char *ex_dividend_date;
} AVCompanyOverview;

/* Company logo URLs returned by COMPANY_LOGO. Strings are heap-allocated (or NULL). */
typedef struct {
    char *symbol;
    char *logo_url_png;
    char *logo_url_svg;
} AVCompanyLogo;

/* Frees all memory owned by an AVTimeSeries and resets its fields.
 * Safe to call on an empty or already-freed series. Returns nothing. */
void av_time_series_free(AVTimeSeries *series);

/* Frees all memory owned by an AVSearchResult and resets its fields.
 * Safe to call on an empty or already-freed result. Returns nothing. */
void av_search_result_free(AVSearchResult *result);

/* Frees all memory owned by an AVMarketStatus and resets its fields.
 * Safe to call on an empty or already-freed status. Returns nothing. */
void av_market_status_free(AVMarketStatus *status);

/* Frees all memory owned by an AVCompanyOverview and resets its fields.
 * Safe to call on an empty or already-freed overview. Returns nothing. */
void av_company_overview_free(AVCompanyOverview *overview);

/* Frees all memory owned by an AVCompanyLogo and resets its fields.
 * Safe to call on an empty or already-freed logo. Returns nothing. */
void av_company_logo_free(AVCompanyLogo *logo);

#ifdef __cplusplus
}
#endif

#endif /* AV_TYPES_H */
