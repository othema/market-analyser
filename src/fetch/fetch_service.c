#include "fetch/fetch_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void stock_fetch_init(StockFetchState *s) {
    memset(s, 0, sizeof(*s));
    pthread_mutex_init(&s->mutex, NULL);
    s->ui_state = FetchUiIdle;
    snprintf(s->status_text, sizeof(s->status_text), "Enter a ticker and click \"View data\".");
}

static void suggestion_init(SuggestionState *s) {
    memset(s, 0, sizeof(*s));
    pthread_mutex_init(&s->mutex, NULL);
}

void fetch_service_init(FetchService *svc, ChartState *chart) {
    if (!svc) {
        return;
    }
    memset(svc, 0, sizeof(*svc));
    svc->chart = chart;
    svc->yf_ready = false;
    stock_fetch_init(&svc->stock);
    suggestion_init(&svc->suggest);
}

bool fetch_service_init_yf(FetchService *svc) {
    if (!svc) {
        return false;
    }
    svc->yf_ready = false;
    if (yf_global_init() != YFOk) {
        fprintf(stderr, "Failed to initialize Yahoo Finance globals\n");
        return false;
    }
    if (yf_client_init(&svc->client) != YFOk) {
        fprintf(stderr, "Failed to initialize Yahoo Finance client\n");
        yf_global_cleanup();
        return false;
    }
    svc->yf_ready = true;
    return true;
}

static void *stock_fetch_thread_main(void *arg) {
    FetchService *svc = (FetchService *)arg;
    if (!svc) {
        return NULL;
    }

    char ticker[16] = {0};
    char interval[8] = {0};
    char range[8] = {0};

    pthread_mutex_lock(&svc->stock.mutex);
    snprintf(ticker, sizeof(ticker), "%s", svc->stock.request_ticker);
    snprintf(interval, sizeof(interval), "%s", svc->stock.request_interval);
    snprintf(range, sizeof(range), "%s", svc->stock.request_range);
    pthread_mutex_unlock(&svc->stock.mutex);

    YFTimeSeries series = {0};
    YFStatus status = yf_get_time_series(&svc->client, ticker, interval, range, &series);
    if (status == YFOk) {
        (void)yf_enrich_company_info(&svc->client, ticker, &series);
    }

    pthread_mutex_lock(&svc->stock.mutex);
    svc->stock.result_status = status;
    svc->stock.result_series = series;
    svc->stock.result_series_valid = (status == YFOk);
    snprintf(svc->stock.result_ticker, sizeof(svc->stock.result_ticker), "%s", ticker);
    snprintf(svc->stock.result_interval, sizeof(svc->stock.result_interval), "%s", interval);
    snprintf(svc->stock.result_range, sizeof(svc->stock.result_range), "%s", range);
    svc->stock.has_result = true;
    svc->stock.in_progress = false;
    pthread_mutex_unlock(&svc->stock.mutex);

    return NULL;
}

bool fetch_service_start_stock(FetchService *svc, const char *ticker, const char *interval, const char *range) {
    if (!svc || !ticker || !interval || !range) {
        return false;
    }
    if (ticker[0] == '\0') {
        pthread_mutex_lock(&svc->stock.mutex);
        svc->stock.ui_state = FetchUiError;
        snprintf(svc->stock.status_text, sizeof(svc->stock.status_text), "Enter a ticker symbol.");
        pthread_mutex_unlock(&svc->stock.mutex);
        return false;
    }
    if (!svc->yf_ready) {
        pthread_mutex_lock(&svc->stock.mutex);
        svc->stock.ui_state = FetchUiError;
        snprintf(svc->stock.status_text, sizeof(svc->stock.status_text),
                 "Yahoo Finance client is not initialized.");
        pthread_mutex_unlock(&svc->stock.mutex);
        return false;
    }

    pthread_mutex_lock(&svc->stock.mutex);
    if (svc->stock.in_progress) {
        pthread_mutex_unlock(&svc->stock.mutex);
        return false;
    }
    if (svc->stock.thread_active) {
        pthread_join(svc->stock.thread, NULL);
        svc->stock.thread_active = false;
    }
    if (svc->stock.has_result && svc->stock.result_series_valid) {
        yf_time_series_free(&svc->stock.result_series);
        memset(&svc->stock.result_series, 0, sizeof(svc->stock.result_series));
    }
    svc->stock.has_result = false;
    svc->stock.result_series_valid = false;
    svc->stock.result_status = YFOk;
    svc->stock.in_progress = true;
    svc->stock.ui_state = FetchUiLoading;
    snprintf(svc->stock.request_ticker, sizeof(svc->stock.request_ticker), "%s", ticker);
    snprintf(svc->stock.request_interval, sizeof(svc->stock.request_interval), "%s", interval);
    snprintf(svc->stock.request_range, sizeof(svc->stock.request_range), "%s", range);
    snprintf(svc->stock.status_text, sizeof(svc->stock.status_text),
             "Fetching %s (%s, %s)...", ticker, interval, range);

    int rc = pthread_create(&svc->stock.thread, NULL, stock_fetch_thread_main, svc);
    if (rc != 0) {
        svc->stock.in_progress = false;
        svc->stock.ui_state = FetchUiError;
        snprintf(svc->stock.status_text, sizeof(svc->stock.status_text),
                 "Failed to start fetch thread.");
        pthread_mutex_unlock(&svc->stock.mutex);
        return false;
    }
    svc->stock.thread_active = true;
    pthread_mutex_unlock(&svc->stock.mutex);
    return true;
}

void fetch_service_poll(FetchService *svc) {
    if (!svc) {
        return;
    }

    pthread_mutex_lock(&svc->stock.mutex);
    if (!svc->stock.has_result || svc->stock.in_progress) {
        pthread_mutex_unlock(&svc->stock.mutex);
        return;
    }

    YFTimeSeries series = svc->stock.result_series;
    YFStatus status = svc->stock.result_status;
    bool series_valid = svc->stock.result_series_valid;
    char ticker[16];
    char interval[8];
    char range[8];
    snprintf(ticker, sizeof(ticker), "%s", svc->stock.result_ticker);
    snprintf(interval, sizeof(interval), "%s", svc->stock.result_interval);
    snprintf(range, sizeof(range), "%s", svc->stock.result_range);

    svc->stock.has_result = false;
    svc->stock.result_series_valid = false;
    memset(&svc->stock.result_series, 0, sizeof(svc->stock.result_series));
    pthread_mutex_unlock(&svc->stock.mutex);

    if (status != YFOk || !series_valid) {
        pthread_mutex_lock(&svc->stock.mutex);
        svc->stock.ui_state = FetchUiError;
        snprintf(svc->stock.status_text, sizeof(svc->stock.status_text),
                 "Failed to fetch %s (%s, %s): %s", ticker, interval, range, yf_status_str(status));
        pthread_mutex_unlock(&svc->stock.mutex);
        yf_time_series_free(&series);
        return;
    }

    if (!svc->chart || !chart_state_set_from_yf_time_series(svc->chart, ticker, &series)) {
        pthread_mutex_lock(&svc->stock.mutex);
        svc->stock.ui_state = FetchUiError;
        snprintf(svc->stock.status_text, sizeof(svc->stock.status_text),
                 "Failed to prepare chart data for %s.", ticker);
        pthread_mutex_unlock(&svc->stock.mutex);
        yf_time_series_free(&series);
        return;
    }

    const char *company_name = ticker;
    if (series.long_name && series.long_name[0] != '\0') {
        company_name = series.long_name;
    } else if (series.short_name && series.short_name[0] != '\0') {
        company_name = series.short_name;
    } else if (series.symbol && series.symbol[0] != '\0') {
        company_name = series.symbol;
    }
    chart_state_set_company_info(svc->chart,
                                 company_name,
                                 series.exchange_name,
                                 series.currency,
                                 series.sector,
                                 series.industry,
                                 NULL);

    pthread_mutex_lock(&svc->stock.mutex);
    svc->stock.ui_state = FetchUiSuccess;
    snprintf(svc->stock.status_text, sizeof(svc->stock.status_text),
             "Loaded %s (%zu points, %s, %s).", ticker, series.count, interval, range);
    pthread_mutex_unlock(&svc->stock.mutex);

    yf_time_series_free(&series);
}

void fetch_service_get_status(FetchService *svc, FetchStatusSnapshot *out) {
    if (!out) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->state = FetchUiIdle;
    snprintf(out->status_text, sizeof(out->status_text), "Application state unavailable.");
    if (!svc) {
        out->state = FetchUiError;
        return;
    }
    pthread_mutex_lock(&svc->stock.mutex);
    out->in_progress = svc->stock.in_progress;
    out->state = svc->stock.ui_state;
    snprintf(out->status_text, sizeof(out->status_text), "%s", svc->stock.status_text);
    pthread_mutex_unlock(&svc->stock.mutex);
}

static void *suggestion_thread_main(void *arg) {
    FetchService *svc = (FetchService *)arg;
    char query[32] = {0};

    pthread_mutex_lock(&svc->suggest.mutex);
    snprintf(query, sizeof(query), "%s", svc->suggest.request_query);
    pthread_mutex_unlock(&svc->suggest.mutex);

    YFSearchResult *results = NULL;
    size_t count = 0;
    if (svc && svc->yf_ready && query[0] != '\0') {
        (void)yf_search_symbols(&svc->client, query, &results, &count, 8);
    }

    pthread_mutex_lock(&svc->suggest.mutex);
    if (svc->suggest.results) {
        yf_search_results_free(svc->suggest.results, svc->suggest.count);
    }
    svc->suggest.results = results;
    svc->suggest.count = count;
    snprintf(svc->suggest.last_applied_query, sizeof(svc->suggest.last_applied_query), "%s", query);
    svc->suggest.in_progress = false;
    pthread_mutex_unlock(&svc->suggest.mutex);
    return NULL;
}

void fetch_service_request_suggestions(FetchService *svc, const char *query) {
    if (!svc || !query) {
        return;
    }

    if (query[0] == '\0') {
        pthread_mutex_lock(&svc->suggest.mutex);
        if (svc->suggest.results) {
            yf_search_results_free(svc->suggest.results, svc->suggest.count);
            svc->suggest.results = NULL;
            svc->suggest.count = 0;
        }
        svc->suggest.last_applied_query[0] = '\0';
        svc->suggest.last_requested_query[0] = '\0';
        pthread_mutex_unlock(&svc->suggest.mutex);
        return;
    }

    pthread_mutex_lock(&svc->suggest.mutex);
    bool same_as_requested = (strncmp(svc->suggest.last_requested_query, query, sizeof(svc->suggest.last_requested_query)) == 0);
    bool busy = svc->suggest.in_progress;
    if (busy || same_as_requested) {
        pthread_mutex_unlock(&svc->suggest.mutex);
        return;
    }
    if (svc->suggest.thread_active) {
        pthread_join(svc->suggest.thread, NULL);
        svc->suggest.thread_active = false;
    }
    snprintf(svc->suggest.request_query, sizeof(svc->suggest.request_query), "%s", query);
    snprintf(svc->suggest.last_requested_query, sizeof(svc->suggest.last_requested_query), "%s", query);
    svc->suggest.in_progress = true;
    int rc = pthread_create(&svc->suggest.thread, NULL, suggestion_thread_main, svc);
    if (rc == 0) {
        svc->suggest.thread_active = true;
    } else {
        svc->suggest.in_progress = false;
        svc->suggest.last_requested_query[0] = '\0';
    }
    pthread_mutex_unlock(&svc->suggest.mutex);
}

void fetch_service_get_suggestions(FetchService *svc, const char *current_query, SuggestionSnapshot *out) {
    if (!out) {
        return;
    }
    memset(out, 0, sizeof(*out));
    if (!svc || !current_query || current_query[0] == '\0') {
        return;
    }

    pthread_mutex_lock(&svc->suggest.mutex);
    out->in_progress = svc->suggest.in_progress;
    bool matches = (strncmp(svc->suggest.last_applied_query, current_query, sizeof(svc->suggest.last_applied_query)) == 0);
    if (matches) {
        size_t view_count = svc->suggest.count < 8 ? svc->suggest.count : 8;
        size_t w = 0;
        for (size_t i = 0; i < view_count && w < 8; i++) {
            YFSearchResult *r = &svc->suggest.results[i];
            if (!r->symbol || r->symbol[0] == '\0') continue;
            snprintf(out->items[w].symbol, sizeof(out->items[w].symbol), "%s", r->symbol);
            const char *name = r->long_name ? r->long_name : (r->short_name ? r->short_name : "");
            snprintf(out->items[w].name, sizeof(out->items[w].name), "%s", name);
            snprintf(out->items[w].exchange, sizeof(out->items[w].exchange), "%s", r->exchange ? r->exchange : "");
            w++;
        }
        out->count = w;
    }
    pthread_mutex_unlock(&svc->suggest.mutex);
}

bool fetch_service_yf_ready(const FetchService *svc) {
    return svc && svc->yf_ready;
}

void fetch_service_shutdown(FetchService *svc) {
    if (!svc) {
        return;
    }

    if (svc->stock.thread_active) {
        pthread_join(svc->stock.thread, NULL);
        svc->stock.thread_active = false;
    }
    if (svc->stock.has_result && svc->stock.result_series_valid) {
        yf_time_series_free(&svc->stock.result_series);
        memset(&svc->stock.result_series, 0, sizeof(svc->stock.result_series));
        svc->stock.has_result = false;
        svc->stock.result_series_valid = false;
    }
    pthread_mutex_destroy(&svc->stock.mutex);

    if (svc->suggest.thread_active) {
        pthread_join(svc->suggest.thread, NULL);
        svc->suggest.thread_active = false;
    }
    if (svc->suggest.results) {
        yf_search_results_free(svc->suggest.results, svc->suggest.count);
        svc->suggest.results = NULL;
        svc->suggest.count = 0;
    }
    pthread_mutex_destroy(&svc->suggest.mutex);

    if (svc->yf_ready) {
        yf_client_cleanup(&svc->client);
        yf_global_cleanup();
        svc->yf_ready = false;
    }
}
