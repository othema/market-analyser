#include "windows/stock_selector.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include "yfinance/yf_chart.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    bool mutex_ready;
    bool thread_active;
    bool in_progress;

    char request_query[32];
    char last_applied_query[32];
    char last_requested_query[32];

    YFSearchResult *results;
    size_t count;
} SuggestionState;

static SuggestionState g_suggest = {0};

static void suggestion_state_ensure_init(void) {
    if (!g_suggest.mutex_ready) {
        pthread_mutex_init(&g_suggest.mutex, NULL);
        g_suggest.mutex_ready = true;
    }
}

static void *suggestion_thread_main(void *arg) {
    AppState *app = (AppState *)arg;
    char query[32] = {0};

    pthread_mutex_lock(&g_suggest.mutex);
    snprintf(query, sizeof(query), "%s", g_suggest.request_query);
    pthread_mutex_unlock(&g_suggest.mutex);

    YFSearchResult *results = NULL;
    size_t count = 0;
    if (app && app->yf.ready && query[0] != '\0') {
        (void)yf_search_symbols(&app->yf.client, query, &results, &count, 8);
    }

    pthread_mutex_lock(&g_suggest.mutex);
    if (g_suggest.results) {
        yf_search_results_free(g_suggest.results, g_suggest.count);
    }
    g_suggest.results = results;
    g_suggest.count = count;
    snprintf(g_suggest.last_applied_query, sizeof(g_suggest.last_applied_query), "%s", query);
    g_suggest.in_progress = false;
    pthread_mutex_unlock(&g_suggest.mutex);
    return NULL;
}

static void suggestion_maybe_start(AppState *app, const char *query) {
    if (!app || !query) {
        return;
    }
    suggestion_state_ensure_init();

    if (query[0] == '\0') {
        pthread_mutex_lock(&g_suggest.mutex);
        if (g_suggest.results) {
            yf_search_results_free(g_suggest.results, g_suggest.count);
            g_suggest.results = NULL;
            g_suggest.count = 0;
        }
        g_suggest.last_applied_query[0] = '\0';
        g_suggest.last_requested_query[0] = '\0';
        pthread_mutex_unlock(&g_suggest.mutex);
        return;
    }

    pthread_mutex_lock(&g_suggest.mutex);
    bool same_as_requested = (strncmp(g_suggest.last_requested_query, query, sizeof(g_suggest.last_requested_query)) == 0);
    bool busy = g_suggest.in_progress;
    if (busy || same_as_requested) {
        pthread_mutex_unlock(&g_suggest.mutex);
        return;
    }
    if (g_suggest.thread_active) {
        pthread_join(g_suggest.thread, NULL);
        g_suggest.thread_active = false;
    }
    snprintf(g_suggest.request_query, sizeof(g_suggest.request_query), "%s", query);
    snprintf(g_suggest.last_requested_query, sizeof(g_suggest.last_requested_query), "%s", query);
    g_suggest.in_progress = true;
    int rc = pthread_create(&g_suggest.thread, NULL, suggestion_thread_main, app);
    if (rc == 0) {
        g_suggest.thread_active = true;
    } else {
        g_suggest.in_progress = false;
        g_suggest.last_requested_query[0] = '\0';
    }
    pthread_mutex_unlock(&g_suggest.mutex);
}

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
    bool input_changed = igInputTextWithHint("##TickerInput", "AAPL", ticker, ticker_size, ImGuiInputTextFlags_None, NULL, NULL);
    bool input_active = igIsItemActive();

    if (app && input_changed) {
        suggestion_maybe_start(app, ticker);
    }
    if (app && ticker && ticker[0] != '\0') {
        suggestion_state_ensure_init();
        pthread_mutex_lock(&g_suggest.mutex);
        bool idle = !g_suggest.in_progress;
        bool different = (strncmp(g_suggest.last_requested_query, ticker, sizeof(g_suggest.last_requested_query)) != 0);
        pthread_mutex_unlock(&g_suggest.mutex);
        if (idle && different) {
            suggestion_maybe_start(app, ticker);
        }
    }

    suggestion_state_ensure_init();
    char applied_query[32] = {0};
    bool suggest_busy = false;
    pthread_mutex_lock(&g_suggest.mutex);
    suggest_busy = g_suggest.in_progress;
    snprintf(applied_query, sizeof(applied_query), "%s", g_suggest.last_applied_query);
    pthread_mutex_unlock(&g_suggest.mutex);

    typedef struct { char symbol[16]; char name[64]; char exchange[32]; } SuggestView;
    SuggestView views[8];
    size_t view_count = 0;
    if (ticker && ticker[0] != '\0' && strncmp(applied_query, ticker, sizeof(applied_query)) == 0) {
        pthread_mutex_lock(&g_suggest.mutex);
        view_count = g_suggest.count < 8 ? g_suggest.count : 8;
        for (size_t i = 0; i < view_count; i++) {
            YFSearchResult *r = &g_suggest.results[i];
            snprintf(views[i].symbol, sizeof(views[i].symbol), "%s", r->symbol ? r->symbol : "");
            const char *name = r->long_name ? r->long_name : (r->short_name ? r->short_name : "");
            snprintf(views[i].name, sizeof(views[i].name), "%s", name);
            snprintf(views[i].exchange, sizeof(views[i].exchange), "%s", r->exchange ? r->exchange : "");
        }
        pthread_mutex_unlock(&g_suggest.mutex);
        size_t w = 0;
        for (size_t i = 0; i < view_count; i++) {
            if (views[i].symbol[0] == '\0') continue;
            if (w != i) views[w] = views[i];
            w++;
        }
        view_count = w;
    }

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

    bool show_suggestions = input_active && ticker && ticker[0] != '\0' &&
                            (suggest_busy || view_count > 0 || strncmp(applied_query, ticker, sizeof(applied_query)) == 0);
    if (show_suggestions) {
        float list_height = 120.0f;
        if (view_count == 0) list_height = 40.0f;
        if (igBeginChild_Str("##TickerSuggestions", (ImVec2){360.0f, list_height}, true, 0)) {
            if (suggest_busy && view_count == 0) {
                igTextDisabled("Searching...");
            } else {
                for (size_t i = 0; i < view_count; i++) {
                    char label[160];
                    if (views[i].exchange[0] != '\0' && views[i].name[0] != '\0') {
                        snprintf(label, sizeof(label), "%s - %s (%s)", views[i].symbol, views[i].name, views[i].exchange);
                    } else if (views[i].name[0] != '\0') {
                        snprintf(label, sizeof(label), "%s - %s", views[i].symbol, views[i].name);
                    } else {
                        snprintf(label, sizeof(label), "%s", views[i].symbol);
                    }
                    igTextUnformatted(label, NULL);
                }
                if (!suggest_busy && view_count == 0 && strncmp(applied_query, ticker, sizeof(applied_query)) == 0) {
                    igTextDisabled("No suggestions");
                }
            }
        }
        igEndChild();
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

