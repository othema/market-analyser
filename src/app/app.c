#include "app/app.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"

#include "plot/plot_c.h"
#include "windows/stock_data.h"
#include "windows/stock_selector.h"

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Logs a GLFW error callback message to stderr.
// Returns nothing.
static void glfw_error_callback(int error, const char *description) {
    (void)error;
    fprintf(stderr, "GLFW error: %s\n", description ? description : "(unknown)");
}

// Initializes shared Yahoo Finance global/client state for the application.
// Returns true on success, or false on initialization failure.
static bool app_init_yf(AppState *app) {
    if (!app) {
        return false;
    }

    app->yf.ready = false;
    if (yf_global_init() != YFOk) {
        fprintf(stderr, "Failed to initialize Yahoo Finance globals\n");
        return false;
    }

    if (yf_client_init(&app->yf.client) != YFOk) {
        fprintf(stderr, "Failed to initialize Yahoo Finance client\n");
        yf_global_cleanup();
        return false;
    }

    app->yf.ready = true;
    return true;
}

// Initializes asynchronous fetch state fields.
// Returns nothing.
static void app_fetch_init(AppState *app) {
    if (!app) {
        return;
    }
    memset(&app->fetch, 0, sizeof(app->fetch));
    pthread_mutex_init(&app->fetch.mutex, NULL);
    app->fetch.ui_state = FetchUiIdle;
    snprintf(app->fetch.status_text, sizeof(app->fetch.status_text),
             "Enter a ticker and click \"View data\".");
}

// Prints a parsed Yahoo Finance time series to stdout for quick inspection/debugging.
// Returns nothing.
static void app_print_series(const char *ticker, const YFTimeSeries *series) {
    if (!ticker || !series) {
        return;
    }

    printf("Stock data for %s (%zu points):\n", ticker, series->count);
    printf("%-12s %10s %10s %10s %10s %12s\n", "date", "open", "high", "low", "close", "volume");
    for (size_t i = 0; i < series->count; i++) {
        const YFTimeSeriesPoint *point = &series->points[i];
        printf("%-12s %10.2f %10.2f %10.2f %10.2f %12lld\n",
               point->date ? point->date : "?",
               point->open,
               point->high,
               point->low,
               point->close,
               point->volume);
    }
    fflush(stdout);
}

// Worker thread function that fetches stock data and stores the result.
// Returns NULL.
static void *app_fetch_thread_main(void *arg) {
    AppState *app = (AppState *)arg;
    if (!app) {
        return NULL;
    }

    char ticker[16] = {0};
    char interval[8] = {0};
    char range[8] = {0};

    pthread_mutex_lock(&app->fetch.mutex);
    snprintf(ticker, sizeof(ticker), "%s", app->fetch.request_ticker);
    snprintf(interval, sizeof(interval), "%s", app->fetch.request_interval);
    snprintf(range, sizeof(range), "%s", app->fetch.request_range);
    pthread_mutex_unlock(&app->fetch.mutex);

    YFTimeSeries series = {0};
    YFStatus status = yf_get_time_series(&app->yf.client, ticker, interval, range, &series);

    pthread_mutex_lock(&app->fetch.mutex);
    app->fetch.result_status = status;
    app->fetch.result_series = series;
    app->fetch.result_series_valid = (status == YFOk);
    snprintf(app->fetch.result_ticker, sizeof(app->fetch.result_ticker), "%s", ticker);
    snprintf(app->fetch.result_interval, sizeof(app->fetch.result_interval), "%s", interval);
    snprintf(app->fetch.result_range, sizeof(app->fetch.result_range), "%s", range);
    app->fetch.has_result = true;
    app->fetch.in_progress = false;
    pthread_mutex_unlock(&app->fetch.mutex);

    return NULL;
}

// Starts an asynchronous stock data fetch if none is in progress.
// Returns true if a fetch was started, or false if busy/invalid.
bool app_start_stock_fetch(AppState *app, const char *ticker, const char *interval, const char *range) {
    if (!app || !ticker || !interval || !range) {
        return false;
    }
    if (ticker[0] == '\0') {
        pthread_mutex_lock(&app->fetch.mutex);
        app->fetch.ui_state = FetchUiError;
        snprintf(app->fetch.status_text, sizeof(app->fetch.status_text), "Enter a ticker symbol.");
        pthread_mutex_unlock(&app->fetch.mutex);
        return false;
    }
    if (!app->yf.ready) {
        pthread_mutex_lock(&app->fetch.mutex);
        app->fetch.ui_state = FetchUiError;
        snprintf(app->fetch.status_text, sizeof(app->fetch.status_text),
                 "Yahoo Finance client is not initialized.");
        pthread_mutex_unlock(&app->fetch.mutex);
        return false;
    }

    pthread_mutex_lock(&app->fetch.mutex);
    if (app->fetch.in_progress) {
        pthread_mutex_unlock(&app->fetch.mutex);
        return false;
    }
    if (app->fetch.thread_active) {
        pthread_join(app->fetch.thread, NULL);
        app->fetch.thread_active = false;
    }
    if (app->fetch.has_result && app->fetch.result_series_valid) {
        yf_time_series_free(&app->fetch.result_series);
        memset(&app->fetch.result_series, 0, sizeof(app->fetch.result_series));
    }
    app->fetch.has_result = false;
    app->fetch.result_series_valid = false;
    app->fetch.result_status = YFOk;
    app->fetch.in_progress = true;
    app->fetch.ui_state = FetchUiLoading;
    snprintf(app->fetch.request_ticker, sizeof(app->fetch.request_ticker), "%s", ticker);
    snprintf(app->fetch.request_interval, sizeof(app->fetch.request_interval), "%s", interval);
    snprintf(app->fetch.request_range, sizeof(app->fetch.request_range), "%s", range);
    snprintf(app->fetch.status_text, sizeof(app->fetch.status_text),
             "Fetching %s (%s, %s)...", ticker, interval, range);

    int rc = pthread_create(&app->fetch.thread, NULL, app_fetch_thread_main, app);
    if (rc != 0) {
        app->fetch.in_progress = false;
        app->fetch.ui_state = FetchUiError;
        snprintf(app->fetch.status_text, sizeof(app->fetch.status_text),
                 "Failed to start fetch thread.");
        pthread_mutex_unlock(&app->fetch.mutex);
        return false;
    }
    app->fetch.thread_active = true;
    pthread_mutex_unlock(&app->fetch.mutex);
    return true;
}

// Polls for completed async fetch results and applies them to app/chart/UI state.
// Returns nothing.
void app_poll_stock_fetch(AppState *app) {
    if (!app) {
        return;
    }

    pthread_mutex_lock(&app->fetch.mutex);
    if (!app->fetch.has_result || app->fetch.in_progress) {
        pthread_mutex_unlock(&app->fetch.mutex);
        return;
    }

    YFTimeSeries series = app->fetch.result_series;
    YFStatus status = app->fetch.result_status;
    bool series_valid = app->fetch.result_series_valid;
    char ticker[16];
    char interval[8];
    char range[8];
    snprintf(ticker, sizeof(ticker), "%s", app->fetch.result_ticker);
    snprintf(interval, sizeof(interval), "%s", app->fetch.result_interval);
    snprintf(range, sizeof(range), "%s", app->fetch.result_range);

    app->fetch.has_result = false;
    app->fetch.result_series_valid = false;
    memset(&app->fetch.result_series, 0, sizeof(app->fetch.result_series));
    pthread_mutex_unlock(&app->fetch.mutex);

    if (status != YFOk || !series_valid) {
        pthread_mutex_lock(&app->fetch.mutex);
        app->fetch.ui_state = FetchUiError;
        snprintf(app->fetch.status_text, sizeof(app->fetch.status_text),
                 "Failed to fetch %s (%s, %s): %s", ticker, interval, range, yf_status_str(status));
        pthread_mutex_unlock(&app->fetch.mutex);
        printf("%s\n", app->fetch.status_text);
        fflush(stdout);
        yf_time_series_free(&series);
        return;
    }

    app_print_series(ticker, &series);

    if (!chart_state_set_from_yf_time_series(&app->chart, ticker, &series)) {
        pthread_mutex_lock(&app->fetch.mutex);
        app->fetch.ui_state = FetchUiError;
        snprintf(app->fetch.status_text, sizeof(app->fetch.status_text),
                 "Failed to prepare chart data for %s.", ticker);
        pthread_mutex_unlock(&app->fetch.mutex);
        printf("%s\n", app->fetch.status_text);
        fflush(stdout);
        yf_time_series_free(&series);
        return;
    }

    const char *company_name = series.symbol ? series.symbol : ticker;
    chart_state_set_company_info(&app->chart, company_name, series.exchange_name, series.currency, NULL, NULL, NULL);

    pthread_mutex_lock(&app->fetch.mutex);
    app->fetch.ui_state = FetchUiSuccess;
    snprintf(app->fetch.status_text, sizeof(app->fetch.status_text),
             "Loaded %s (%zu points, %s, %s).", ticker, series.count, interval, range);
    pthread_mutex_unlock(&app->fetch.mutex);
    printf("%s\n", app->fetch.status_text);
    fflush(stdout);

    yf_time_series_free(&series);
}

// Creates the GLFW window and OpenGL context for the application.
// Returns true on success, or false on failure.
static bool app_init_window(AppState *app) {
    if (!app) {
        return false;
    }

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

#if defined(__APPLE__)
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    app->window = glfwCreateWindow(1280, 720, "Market Analyzer", NULL, NULL);
    if (!app->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(app->window);
    glfwSwapInterval(1);

    igCreateContext(NULL);
    plot_create_context();

    ImGuiIO *io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    igStyleColorsDark(NULL);

    ImGui_ImplGlfw_InitForOpenGL(app->window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    (void)glsl_version;
    return true;
}

// Initializes application state, platform/windowing, ImGui/ImPlot, and API client.
// Returns true on success, or false if initialization fails.
bool app_init(AppState *app) {
    if (!app) {
        return false;
    }

    app->window = NULL;
    chart_state_init(&app->chart);
    app_fetch_init(app);

    if (!app_init_yf(app)) {
        return false;
    }
    if (!app_init_window(app)) {
        pthread_mutex_destroy(&app->fetch.mutex);
        yf_client_cleanup(&app->yf.client);
        yf_global_cleanup();
        app->yf.ready = false;
        return false;
    }

    return true;
}

// Renders the integrated application UI within a single main window.
// Returns nothing.
static void app_render(AppState *app) {
    static char ticker[16] = "";

    app_poll_stock_fetch(app);

    ImGuiIO *io = igGetIO_Nil();
    ImVec2 display_size = io ? io->DisplaySize : (ImVec2){1280.0f, 720.0f};
    igSetNextWindowPos((ImVec2){0.0f, 0.0f}, ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
    igSetNextWindowSize(display_size, ImGuiCond_Always);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (igBegin("Market Analyzer", NULL, window_flags)) {

        if (igBeginChild_Str("ChartSection", (ImVec2){0.0f, -100.0f}, true, 0)) {
            stock_data_render(app);
        }
        igEndChild();

        if (igBeginChild_Str("SelectorSection", (ImVec2){0.0f, 0.0f}, true, 0)) {
            stock_selector_render(app, ticker, (int)sizeof(ticker));
        }
        igEndChild();
    }
    igEnd();
}

// Begins a new ImGui frame.
// Returns nothing.
static void app_begin_frame(void) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();
}

// Renders ImGui draw data and swaps the window buffers for the current frame.
// Returns nothing.
static void app_end_frame(AppState *app) {
    if (!app || !app->window) {
        return;
    }

    igRender();

    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(app->window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.10f, 0.10f, 0.12f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
    glfwSwapBuffers(app->window);
}

// Runs the main application/render loop until the window is closed.
// Returns nothing.
void app_run(AppState *app) {
    if (!app || !app->window) {
        return;
    }

    while (!glfwWindowShouldClose(app->window)) {
        glfwPollEvents();
        app_begin_frame();
        app_render(app);
        app_end_frame(app);
    }
}

// Shuts down and releases all resources owned by the application.
// Safe to call after partial initialization. Returns nothing.
void app_shutdown(AppState *app) {
    if (!app) {
        return;
    }

    if (app->fetch.thread_active) {
        pthread_join(app->fetch.thread, NULL);
        app->fetch.thread_active = false;
    }
    if (app->fetch.has_result && app->fetch.result_series_valid) {
        yf_time_series_free(&app->fetch.result_series);
        memset(&app->fetch.result_series, 0, sizeof(app->fetch.result_series));
        app->fetch.has_result = false;
        app->fetch.result_series_valid = false;
    }
    pthread_mutex_destroy(&app->fetch.mutex);

    if (app->window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        plot_destroy_context();
        igDestroyContext(NULL);

        glfwDestroyWindow(app->window);
        app->window = NULL;
        glfwTerminate();
    }

    if (app->yf.ready) {
        yf_client_cleanup(&app->yf.client);
        yf_global_cleanup();
        app->yf.ready = false;
    }

    chart_state_clear(&app->chart);
}

