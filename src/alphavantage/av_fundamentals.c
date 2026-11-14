#include "av_fundamentals.h"
#include "av_utils.h"

#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

/* Frees all memory owned by an AVCompanyOverview and resets its fields.
 * Safe to call on an empty or already-freed overview. Returns nothing. */
void av_company_overview_free(AVCompanyOverview *overview) {
    if (!overview) return;
    free(overview->symbol);
    free(overview->asset_type);
    free(overview->name);
    free(overview->description);
    free(overview->cik);
    free(overview->exchange);
    free(overview->currency);
    free(overview->country);
    free(overview->sector);
    free(overview->industry);
    free(overview->address);
    free(overview->fiscal_year_end);
    free(overview->latest_quarter);
    free(overview->market_capitalization);
    free(overview->ebitda);
    free(overview->pe_ratio);
    free(overview->peg_ratio);
    free(overview->book_value);
    free(overview->dividend_per_share);
    free(overview->dividend_yield);
    free(overview->eps);
    free(overview->revenue_per_share_ttm);
    free(overview->profit_margin);
    free(overview->operating_margin_ttm);
    free(overview->return_on_assets_ttm);
    free(overview->return_on_equity_ttm);
    free(overview->revenue_ttm);
    free(overview->gross_profit_ttm);
    free(overview->diluted_eps_ttm);
    free(overview->quarterly_earnings_growth_yoy);
    free(overview->quarterly_revenue_growth_yoy);
    free(overview->analyst_target_price);
    free(overview->trailing_pe);
    free(overview->forward_pe);
    free(overview->price_to_sales_ratio_ttm);
    free(overview->price_to_book_ratio);
    free(overview->ev_to_revenue);
    free(overview->ev_to_ebitda);
    free(overview->beta);
    free(overview->week_52_high);
    free(overview->week_52_low);
    free(overview->day_50_moving_average);
    free(overview->day_200_moving_average);
    free(overview->shares_outstanding);
    free(overview->dividend_date);
    free(overview->ex_dividend_date);
    /* reset all fields to NULL/zero */
    /* simplest: memset not used to avoid surprising padding; set key pointer already freed */
    overview->symbol = NULL;
    overview->asset_type = NULL;
    overview->name = NULL;
    overview->description = NULL;
    overview->cik = NULL;
    overview->exchange = NULL;
    overview->currency = NULL;
    overview->country = NULL;
    overview->sector = NULL;
    overview->industry = NULL;
    overview->address = NULL;
    overview->fiscal_year_end = NULL;
    overview->latest_quarter = NULL;
    overview->market_capitalization = NULL;
    overview->ebitda = NULL;
    overview->pe_ratio = NULL;
    overview->peg_ratio = NULL;
    overview->book_value = NULL;
    overview->dividend_per_share = NULL;
    overview->dividend_yield = NULL;
    overview->eps = NULL;
    overview->revenue_per_share_ttm = NULL;
    overview->profit_margin = NULL;
    overview->operating_margin_ttm = NULL;
    overview->return_on_assets_ttm = NULL;
    overview->return_on_equity_ttm = NULL;
    overview->revenue_ttm = NULL;
    overview->gross_profit_ttm = NULL;
    overview->diluted_eps_ttm = NULL;
    overview->quarterly_earnings_growth_yoy = NULL;
    overview->quarterly_revenue_growth_yoy = NULL;
    overview->analyst_target_price = NULL;
    overview->trailing_pe = NULL;
    overview->forward_pe = NULL;
    overview->price_to_sales_ratio_ttm = NULL;
    overview->price_to_book_ratio = NULL;
    overview->ev_to_revenue = NULL;
    overview->ev_to_ebitda = NULL;
    overview->beta = NULL;
    overview->week_52_high = NULL;
    overview->week_52_low = NULL;
    overview->day_50_moving_average = NULL;
    overview->day_200_moving_average = NULL;
    overview->shares_outstanding = NULL;
    overview->dividend_date = NULL;
    overview->ex_dividend_date = NULL;
}

/* Frees all memory owned by an AVCompanyLogo and resets its fields.
 * Safe to call on an empty or already-freed logo. Returns nothing. */
void av_company_logo_free(AVCompanyLogo *logo) {
    if (!logo) return;
    free(logo->symbol);
    free(logo->logo_url_png);
    free(logo->logo_url_svg);
    logo->symbol = NULL;
    logo->logo_url_png = NULL;
    logo->logo_url_svg = NULL;
}

/* Parses an OVERVIEW JSON response body into an AVCompanyOverview.
 * On success, allocates and fills *out (caller must free with av_company_overview_free).
 * Returns AVOk on success, AVErrInvalidArg for bad inputs, AVErrNomem on
 * allocation failure, or AVErrCurl if the content is not a valid overview. */
AVStatus av_parse_company_overview(const char *json_data, AVCompanyOverview *out) {
    if (!json_data || !out) return AVErrInvalidArg;
    memset(out, 0, sizeof(*out));
    if (av_response_is_error(json_data)) return AVErrCurl;

    struct json_object *root = json_tokener_parse(json_data);
    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) json_object_put(root);
        return AVErrCurl;
    }

    /* Require at least Symbol to consider it a valid overview payload. */
    struct json_object *sym = NULL;
    if (!json_object_object_get_ex(root, "Symbol", &sym)) {
        json_object_put(root);
        return AVErrCurl;
    }

    out->symbol = av_json_get_string_dup(root, "Symbol");
    out->asset_type = av_json_get_string_dup(root, "AssetType");
    out->name = av_json_get_string_dup(root, "Name");
    out->description = av_json_get_string_dup(root, "Description");
    out->cik = av_json_get_string_dup(root, "CIK");
    out->exchange = av_json_get_string_dup(root, "Exchange");
    out->currency = av_json_get_string_dup(root, "Currency");
    out->country = av_json_get_string_dup(root, "Country");
    out->sector = av_json_get_string_dup(root, "Sector");
    out->industry = av_json_get_string_dup(root, "Industry");
    out->address = av_json_get_string_dup(root, "Address");
    out->fiscal_year_end = av_json_get_string_dup(root, "FiscalYearEnd");
    out->latest_quarter = av_json_get_string_dup(root, "LatestQuarter");
    out->market_capitalization = av_json_get_string_dup(root, "MarketCapitalization");
    out->ebitda = av_json_get_string_dup(root, "EBITDA");
    out->pe_ratio = av_json_get_string_dup(root, "PERatio");
    out->peg_ratio = av_json_get_string_dup(root, "PEGRatio");
    out->book_value = av_json_get_string_dup(root, "BookValue");
    out->dividend_per_share = av_json_get_string_dup(root, "DividendPerShare");
    out->dividend_yield = av_json_get_string_dup(root, "DividendYield");
    out->eps = av_json_get_string_dup(root, "EPS");
    out->revenue_per_share_ttm = av_json_get_string_dup(root, "RevenuePerShareTTM");
    out->profit_margin = av_json_get_string_dup(root, "ProfitMargin");
    out->operating_margin_ttm = av_json_get_string_dup(root, "OperatingMarginTTM");
    out->return_on_assets_ttm = av_json_get_string_dup(root, "ReturnOnAssetsTTM");
    out->return_on_equity_ttm = av_json_get_string_dup(root, "ReturnOnEquityTTM");
    out->revenue_ttm = av_json_get_string_dup(root, "RevenueTTM");
    out->gross_profit_ttm = av_json_get_string_dup(root, "GrossProfitTTM");
    out->diluted_eps_ttm = av_json_get_string_dup(root, "DilutedEPSTTM");
    out->quarterly_earnings_growth_yoy = av_json_get_string_dup(root, "QuarterlyEarningsGrowthYOY");
    out->quarterly_revenue_growth_yoy = av_json_get_string_dup(root, "QuarterlyRevenueGrowthYOY");
    out->analyst_target_price = av_json_get_string_dup(root, "AnalystTargetPrice");
    out->trailing_pe = av_json_get_string_dup(root, "TrailingPE");
    out->forward_pe = av_json_get_string_dup(root, "ForwardPE");
    out->price_to_sales_ratio_ttm = av_json_get_string_dup(root, "PriceToSalesRatioTTM");
    out->price_to_book_ratio = av_json_get_string_dup(root, "PriceToBookRatio");
    out->ev_to_revenue = av_json_get_string_dup(root, "EVToRevenue");
    out->ev_to_ebitda = av_json_get_string_dup(root, "EVToEBITDA");
    out->beta = av_json_get_string_dup(root, "Beta");
    out->week_52_high = av_json_get_string_dup(root, "52WeekHigh");
    out->week_52_low = av_json_get_string_dup(root, "52WeekLow");
    out->day_50_moving_average = av_json_get_string_dup(root, "50DayMovingAverage");
    out->day_200_moving_average = av_json_get_string_dup(root, "200DayMovingAverage");
    out->shares_outstanding = av_json_get_string_dup(root, "SharesOutstanding");
    out->dividend_date = av_json_get_string_dup(root, "DividendDate");
    out->ex_dividend_date = av_json_get_string_dup(root, "ExDividendDate");

    json_object_put(root);
    return AVOk;
}

/* Fetches and parses company overview data (OVERVIEW) for a symbol.
 * On success, fills *out (caller must free with av_company_overview_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_company_overview(AVClient *client, const char *symbol, AVCompanyOverview *out) {
    if (!client || !symbol || !out) return AVErrInvalidArg;
    memset(out, 0, sizeof(*out));
    AVParam params[] = { {"symbol", symbol} };
    AVResponse resp;
    av_response_init(&resp);
    AVStatus st = av_query(client, "OVERVIEW", params, 1, &resp);
    if (st != AVOk) { av_response_free(&resp); return st; }
    if (resp.http_code != 200 || !resp.data) { av_response_free(&resp); return AVErrCurl; }
    st = av_parse_company_overview(resp.data, out);
    av_response_free(&resp);
    return st;
}

/* Parses a COMPANY_LOGO JSON response body into an AVCompanyLogo.
 * On success, allocates and fills *out (caller must free with av_company_logo_free).
 * Returns AVOk on success, AVErrInvalidArg for bad inputs, AVErrNomem on
 * allocation failure, or AVErrCurl if the content is not a valid logo response. */
AVStatus av_parse_company_logo(const char *json_data, AVCompanyLogo *out) {
    if (!json_data || !out) return AVErrInvalidArg;
    memset(out, 0, sizeof(*out));
    if (av_response_is_error(json_data)) return AVErrCurl;

    struct json_object *root = json_tokener_parse(json_data);
    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) json_object_put(root);
        return AVErrCurl;
    }

    /* Expect at least symbol or logo fields. */
    if (!json_object_object_get_ex(root, "symbol", NULL) &&
        !json_object_object_get_ex(root, "logo_url_png", NULL) &&
        !json_object_object_get_ex(root, "logo_url_svg", NULL)) {
        json_object_put(root);
        return AVErrCurl;
    }

    out->symbol = av_json_get_string_dup(root, "symbol");
    out->logo_url_png = av_json_get_string_dup(root, "logo_url_png");
    out->logo_url_svg = av_json_get_string_dup(root, "logo_url_svg");

    json_object_put(root);
    return AVOk;
}

/* Fetches and parses company logo URLs (COMPANY_LOGO) for a symbol.
 * On success, fills *out (caller must free with av_company_logo_free).
 * Returns AVOk on success, or an AVErr* status on failure. */
AVStatus av_get_company_logo(AVClient *client, const char *symbol, AVCompanyLogo *out) {
    if (!client || !symbol || !out) return AVErrInvalidArg;
    memset(out, 0, sizeof(*out));
    AVParam params[] = { {"symbol", symbol} };
    AVResponse resp;
    av_response_init(&resp);
    AVStatus st = av_query(client, "COMPANY_LOGO", params, 1, &resp);
    if (st != AVOk) { av_response_free(&resp); return st; }
    if (resp.http_code != 200 || !resp.data) { av_response_free(&resp); return AVErrCurl; }
    st = av_parse_company_logo(resp.data, out);
    av_response_free(&resp);
    return st;
}
