#include "app/app.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include "windows/stock_data.h"
#include "windows/stock_selector.h"

#include <stdio.h>

static void app_render(AppState *app) {
    static char ticker[16] = "";

    if (!app) {
        return;
    }

    fetch_service_poll(&app->fetch);

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

bool app_init(AppState *app) {
    if (!app) {
        return false;
    }

    chart_state_init(&app->chart);
    fetch_service_init(&app->fetch, &app->chart);

    if (!fetch_service_init_yf(&app->fetch)) {
        return false;
    }
    if (!ui_backend_init(&app->ui, "Market Analyzer", 1280, 720)) {
        fetch_service_shutdown(&app->fetch);
        chart_state_clear(&app->chart);
        return false;
    }

    return true;
}

void app_run(AppState *app) {
    if (!app) {
        return;
    }
    while (!ui_backend_should_close(&app->ui)) {
        ui_backend_begin_frame(&app->ui);
        app_render(app);
        ui_backend_end_frame(&app->ui);
    }
}

void app_shutdown(AppState *app) {
    if (!app) {
        return;
    }
    ui_backend_shutdown(&app->ui);
    fetch_service_shutdown(&app->fetch);
    chart_state_clear(&app->chart);
}
