#include "windows/stock_selector.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

// Renders stock selector controls and handles asynchronous fetch requests from the UI.
// Intended to be called inside an existing ImGui window/child region.
// Returns nothing.
void stock_selector_render(AppState *app, char *ticker, int ticker_size) {
    static int range_index = 2;    // 6mo
    static int interval_index = 0; // 1d

    const char *range_options[] = {"1mo", "3mo", "6mo", "1y", "5y", "max"};
    const char *interval_options[] = {"1d", "1wk", "1mo"};
    const int range_count = (int)(sizeof(range_options) / sizeof(range_options[0]));
    const int interval_count = (int)(sizeof(interval_options) / sizeof(interval_options[0]));

    FetchUiState status_state = FetchUiIdle;
    char status_text[256] = {0};
    bool in_progress = false;

    if (app) {
        pthread_mutex_lock(&app->fetch.mutex);
        status_state = app->fetch.ui_state;
        in_progress = app->fetch.in_progress;
        snprintf(status_text, sizeof(status_text), "%s", app->fetch.status_text);
        pthread_mutex_unlock(&app->fetch.mutex);
    } else {
        snprintf(status_text, sizeof(status_text), "Application state unavailable.");
        status_state = FetchUiError;
    }

    igTextUnformatted("Symbol", NULL);
    igSetNextItemWidth(180.0f);
    igInputTextWithHint("##TickerInput", "AAPL", ticker, ticker_size, ImGuiInputTextFlags_None, NULL, NULL);
    igSameLine(0.0f, 8.0f);

    igSetNextItemWidth(90.0f);
    igCombo_Str_arr("Range", &range_index, range_options, range_count, range_count);
    igSameLine(0.0f, 8.0f);

    igSetNextItemWidth(90.0f);
    igCombo_Str_arr("Interval", &interval_index, interval_options, interval_count, interval_count);
    igSameLine(0.0f, 8.0f);

    bool can_fetch = app && !in_progress;
    if (!can_fetch) {
        igBeginDisabled(true);
    }
    if (igButton(in_progress ? "Loading..." : "View data", (ImVec2){0, 0})) {
        if (app) {
            app_start_stock_fetch(app, ticker, interval_options[interval_index], range_options[range_index]);
        }
    }
    if (!can_fetch) {
        igEndDisabled();
    }

    if (app && app->yf.ready) {
        igTextDisabled("Yahoo Finance client ready");
    } else {
        igTextDisabled("Yahoo Finance client not initialized");
    }

    if (status_state == FetchUiSuccess) {
        igTextColored((ImVec4){0.40f, 0.85f, 0.45f, 1.0f}, "%s", status_text);
    } else if (status_state == FetchUiError) {
        igTextColored((ImVec4){0.95f, 0.45f, 0.45f, 1.0f}, "%s", status_text);
    } else if (status_state == FetchUiLoading) {
        igTextColored((ImVec4){0.90f, 0.80f, 0.30f, 1.0f}, "%s", status_text);
    } else {
        igTextWrapped("%s", status_text);
    }
}

