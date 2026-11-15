#ifndef APP_H
#define APP_H

#include <stdbool.h>

#include "chart/chart.h"
#include "fetch/fetch_service.h"
#include "platform/ui_backend.h"

// Holds top-level application state shared across windows and modules.
typedef struct {
    UiBackend ui;
    FetchService fetch;
    ChartState chart;
} AppState;

// Initializes application state, platform/windowing, ImGui/ImPlot, and API client.
// Returns true on success, or false if initialization fails.
bool app_init(AppState *app);

// Runs the main application/render loop until the window is closed.
// Returns nothing.
void app_run(AppState *app);

// Shuts down and releases all resources owned by the application.
// Safe to call after partial initialization. Returns nothing.
void app_shutdown(AppState *app);

#endif // APP_H
