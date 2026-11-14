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

// Logs a GLFW error callback message to stderr.
// Returns nothing.
static void glfw_error_callback(int error, const char *description) {
    (void)error;
    fprintf(stderr, "GLFW error: %s\n", description ? description : "(unknown)");
}

// Initializes shared Alpha Vantage global/client state for the application.
// Returns true on success, or false on initialization failure.
static bool app_init_av(AppState *app) {
    if (!app) {
        return false;
    }

    app->av.ready = false;
    if (av_global_init() != AVOk) {
        fprintf(stderr, "Failed to initialize Alpha Vantage globals\n");
        return false;
    }

    const char *api_key = getenv("ALPHAVANTAGE_API_KEY");
    if (av_client_init(&app->av.client, api_key) != AVOk) {
        fprintf(stderr, "Failed to initialize Alpha Vantage client\n");
        av_global_cleanup();
        return false;
    }
    if (!api_key || api_key[0] == '\0') {
        fprintf(stderr, "Warning: ALPHAVANTAGE_API_KEY is not set; API requests will fail\n");
    }

    app->av.ready = true;
    return true;
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

    if (!app_init_av(app)) {
        return false;
    }
    if (!app_init_window(app)) {
        av_client_cleanup(&app->av.client);
        av_global_cleanup();
        app->av.ready = false;
        return false;
    }

    return true;
}

// Renders all application windows for the current frame.
// Returns nothing.
static void app_render(AppState *app) {
    static char ticker[16] = "";

    stock_selector_render(app, ticker, (int)sizeof(ticker));
    stock_data_render(app);
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

    if (app->window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        plot_destroy_context();
        igDestroyContext(NULL);

        glfwDestroyWindow(app->window);
        app->window = NULL;
        glfwTerminate();
    }

    if (app->av.ready) {
        av_client_cleanup(&app->av.client);
        av_global_cleanup();
        app->av.ready = false;
    }

    chart_state_clear(&app->chart);
}

