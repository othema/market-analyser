#ifndef FETCH_SERVICE_H
#define FETCH_SERVICE_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include "chart/chart.h"
#include "yfinance/yfinance.h"

typedef enum {
    FetchUiIdle = 0,
    FetchUiLoading,
    FetchUiSuccess,
    FetchUiError
} FetchUiState;

typedef struct {
    bool in_progress;
    FetchUiState state;
    char status_text[256];
} FetchStatusSnapshot;

typedef struct {
    char symbol[16];
    char name[64];
    char exchange[32];
} SuggestionView;

typedef struct {
    bool in_progress;
    size_t count;
    SuggestionView items[8];
} SuggestionSnapshot;

typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    bool thread_active;
    bool in_progress;
    bool has_result;

    char request_ticker[16];
    char request_interval[8];
    char request_range[8];

    YFStatus result_status;
    YFTimeSeries result_series;
    bool result_series_valid;
    char result_ticker[16];
    char result_interval[8];
    char result_range[8];

    FetchUiState ui_state;
    char status_text[256];
} StockFetchState;

typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    bool thread_active;
    bool in_progress;

    char request_query[32];
    char last_applied_query[32];
    char last_requested_query[32];

    YFSearchResult *results;
    size_t count;
} SuggestionState;

typedef struct {
    YFClient client;
    bool yf_ready;

    StockFetchState stock;
    SuggestionState suggest;

    ChartState *chart; // not owned
} FetchService;

void fetch_service_init(FetchService *svc, ChartState *chart);
bool fetch_service_init_yf(FetchService *svc);
void fetch_service_shutdown(FetchService *svc);

bool fetch_service_start_stock(FetchService *svc, const char *ticker, const char *interval, const char *range);
void fetch_service_poll(FetchService *svc);
void fetch_service_get_status(const FetchService *svc, FetchStatusSnapshot *out);

void fetch_service_request_suggestions(FetchService *svc, const char *query);
void fetch_service_get_suggestions(const FetchService *svc, const char *current_query, SuggestionSnapshot *out);
bool fetch_service_yf_ready(const FetchService *svc);

#endif // FETCH_SERVICE_H
