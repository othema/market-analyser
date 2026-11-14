#ifndef APP_H
#define APP_H

#include <stdbool.h>

#include "alphavantage/alphavantage.h"
#include "chart/chart.h"

typedef struct GLFWwindow GLFWwindow;

// Holds shared Alpha Vantage client state owned by the application.
typedef struct {
    AVClient client;
    bool ready;
} AVState;

// Holds top-level application state shared across windows and modules.
typedef struct {
    GLFWwindow *window;
    AVState av;
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

