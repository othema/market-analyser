#include "av_fundamentals.h"
#include "av_utils.h"

#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

// Frees all memory owned by an AVCompanyOverview and resets its fields.
// Safe to call on an empty or already-freed overview. Returns nothing.
void av_company_overview_free(AVCompanyOverview *overview) {
    if (!overview) {
        return;
    }

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

    memset(overview, 0, sizeof(*overview));
}

// Parses an OVERVIEW JSON response body into an AVCompanyOverview.
// On success, fills *out (caller must free with av_company_overview_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_parse_company_overview(const char *json_data, AVCompanyOverview *out) {
    if (!json_data || !out) {
        return AVErrInvalidArg;
    }

    memset(out, 0, sizeof(*out));
    if (av_response_is_error(json_data)) {
        return AVErrCurl;
    }

    struct json_object *root = json_tokener_parse(json_data);
    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) {
            json_object_put(root);
        }
        return AVErrCurl;
    }

    // Require at least Symbol to consider it a valid overview payload.
    if (!json_object_object_get_ex(root, "Symbol", NULL)) {
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

// Fetches and parses company overview data (OVERVIEW) for a symbol.
// On success, fills *out (caller must free with av_company_overview_free).
// Returns AVOk on success, or an AVErr* status on failure.
AVStatus av_get_company_overview(AVClient *client, const char *symbol, AVCompanyOverview *out) {
    if (!client || !symbol || !out) {
        return AVErrInvalidArg;
    }

    memset(out, 0, sizeof(*out));
    AVParam params[] = {{"symbol", symbol}};
    AVResponse response;
    av_response_init(&response);

    AVStatus status = av_query(client, "OVERVIEW", params, 1, &response);
    if (status != AVOk) {
        av_response_free(&response);
        return status;
    }
    if (response.http_code != 200 || !response.data) {
        av_response_free(&response);
        return AVErrCurl;
    }

    status = av_parse_company_overview(response.data, out);
    av_response_free(&response);
    return status;
}
