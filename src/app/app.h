#ifndef APP_H
#define APP_H

#include <pthread.h>
#include <stdbool.h>

#include "chart/chart.h"
#include "yfinance/yfinance.h"

typedef struct GLFWwindow GLFWwindow;

// Holds shared Yahoo Finance client state owned by the application.
typedef struct {
    YFClient client;
    bool ready;
} YFState;

typedef enum {
    FetchUiIdle = 0,
    FetchUiLoading,
    FetchUiSuccess,
    FetchUiError
} FetchUiState;

// Holds asynchronous stock fetch state shared between UI and worker thread.
typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    bool thread_active;
    bool in_progress;
    bool has_result;

    // Request/result fields (protected by mutex where relevant).
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
} FetchState;

// Holds top-level application state shared across windows and modules.
typedef struct {
    GLFWwindow *window;
    YFState yf;
    ChartState chart;
    FetchState fetch;
} AppState;

// Initializes application state, platform/windowing, ImGui/ImPlot, and API client.
// Returns true on success, or false if initialization fails.
bool app_init(AppState *app);

// Runs the main application/render loop until the window is closed.
// Returns nothing.
void app_run(AppState *app);

// Starts an asynchronous stock data fetch if none is in progress.
// Returns true if a fetch was started, or false if busy/invalid.
bool app_start_stock_fetch(AppState *app, const char *ticker, const char *interval, const char *range);

// Polls for completed async fetch results and applies them to app/chart/UI state.
// Returns nothing.
void app_poll_stock_fetch(AppState *app);

// Shuts down and releases all resources owned by the application.
// Safe to call after partial initialization. Returns nothing.
void app_shutdown(AppState *app);

#endif // APP_H

