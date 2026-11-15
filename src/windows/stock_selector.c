#include "windows/stock_selector.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include <stdio.h>
#include <string.h>

void stock_selector_render(AppState *app, char *ticker, int ticker_size) {
    static int range_index = 2;    // 6mo
    static int interval_index = 0; // 1d

    const char *range_options[] = {"1mo", "3mo", "6mo", "1y", "5y", "max"};
    const char *interval_options[] = {"1d", "1wk", "1mo"};
    const int range_count = (int)(sizeof(range_options) / sizeof(range_options[0]));
    const int interval_count = (int)(sizeof(interval_options) / sizeof(interval_options[0]));

    FetchStatusSnapshot status = {0};
    if (app) {
        fetch_service_get_status(&app->fetch, &status);
    } else {
        status.state = FetchUiError;
        snprintf(status.status_text, sizeof(status.status_text), "Application state unavailable.");
    }

    igTextUnformatted("Symbol", NULL);
    igSetNextItemWidth(180.0f);
    bool input_changed = igInputTextWithHint("##TickerInput", "AAPL", ticker, ticker_size, ImGuiInputTextFlags_None, NULL, NULL);
    bool input_active = igIsItemActive();

    if (app && input_changed) {
        fetch_service_request_suggestions(&app->fetch, ticker);
    }
    if (app && ticker && ticker[0] != '\0' && input_changed) {
        // request already sent above when changed
    } else if (app && ticker && ticker[0] != '\0') {
        // allow retry/start if query differs from last requested/applied in service snapshot terms
        fetch_service_request_suggestions(&app->fetch, ticker);
    }

    SuggestionSnapshot suggestions = {0};
    if (app) {
        fetch_service_get_suggestions(&app->fetch, ticker ? ticker : "", &suggestions);
    }

    igSameLine(0.0f, 8.0f);
    igSetNextItemWidth(90.0f);
    igCombo_Str_arr("Range", &range_index, range_options, range_count, range_count);
    igSameLine(0.0f, 8.0f);

    igSetNextItemWidth(90.0f);
    igCombo_Str_arr("Interval", &interval_index, interval_options, interval_count, interval_count);
    igSameLine(0.0f, 8.0f);

    bool can_fetch = app && !status.in_progress;
    if (!can_fetch) {
        igBeginDisabled(true);
    }
    if (igButton(status.in_progress ? "Loading..." : "Fetch data", (ImVec2){0, 0})) {
        if (app) {
            fetch_service_start_stock(&app->fetch, ticker, interval_options[interval_index], range_options[range_index]);
        }
    }
    if (!can_fetch) {
        igEndDisabled();
    }

    bool show_suggestions = input_active && ticker && ticker[0] != '\0' &&
                            (suggestions.in_progress || suggestions.count > 0);
    if (show_suggestions) {
        float list_height = 120.0f;
        if (suggestions.count == 0) list_height = 40.0f;
        if (igBeginChild_Str("##TickerSuggestions", (ImVec2){360.0f, list_height}, true, 0)) {
            if (suggestions.in_progress && suggestions.count == 0) {
                igTextDisabled("Searching...");
            } else {
                for (size_t i = 0; i < suggestions.count; i++) {
                    char label[160];
                    const SuggestionView *v = &suggestions.items[i];
                    if (v->exchange[0] != '\0' && v->name[0] != '\0') {
                        snprintf(label, sizeof(label), "%s - %s (%s)", v->symbol, v->name, v->exchange);
                    } else if (v->name[0] != '\0') {
                        snprintf(label, sizeof(label), "%s - %s", v->symbol, v->name);
                    } else {
                        snprintf(label, sizeof(label), "%s", v->symbol);
                    }
                    igTextUnformatted(label, NULL);
                }
                if (!suggestions.in_progress && suggestions.count == 0) {
                    igTextDisabled("No suggestions");
                }
            }
        }
        igEndChild();
    }

    if (app && fetch_service_yf_ready(&app->fetch)) {
        igTextDisabled("Yahoo Finance client ready");
    } else {
        igTextDisabled("Yahoo Finance client not initialized");
    }

    if (status.state == FetchUiSuccess) {
        igTextColored((ImVec4){0.40f, 0.85f, 0.45f, 1.0f}, "%s", status.status_text);
    } else if (status.state == FetchUiError) {
        igTextColored((ImVec4){0.95f, 0.45f, 0.45f, 1.0f}, "%s", status.status_text);
    } else if (status.state == FetchUiLoading) {
        igTextColored((ImVec4){0.90f, 0.80f, 0.30f, 1.0f}, "%s", status.status_text);
    } else {
        igTextWrapped("%s", status.status_text);
    }
}
