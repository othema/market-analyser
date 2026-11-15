#include "platform/ui_backend.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"

#include "plot/plot_c.h"

#include <GLFW/glfw3.h>
#include <stdio.h>

static void glfw_error_callback(int error, const char *description) {
    (void)error;
    fprintf(stderr, "GLFW error: %s\n", description ? description : "(unknown)");
}

bool ui_backend_init(UiBackend *ui, const char *title, int width, int height) {
    if (!ui) {
        return false;
    }
    ui->window = NULL;
    ui->imgui_ready = false;

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

    ui->window = glfwCreateWindow(width, height, title ? title : "Market Analyzer", NULL, NULL);
    if (!ui->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(ui->window);
    glfwSwapInterval(1);

    igCreateContext(NULL);
    plot_create_context();

    ImGuiIO *io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    igStyleColorsDark(NULL);

    ImGui_ImplGlfw_InitForOpenGL(ui->window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    (void)glsl_version;

    ui->imgui_ready = true;
    return true;
}

bool ui_backend_should_close(const UiBackend *ui) {
    return !ui || !ui->window || glfwWindowShouldClose(ui->window);
}

void ui_backend_begin_frame(UiBackend *ui) {
    if (!ui || !ui->window) {
        return;
    }
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();
}

void ui_backend_end_frame(UiBackend *ui) {
    if (!ui || !ui->window) {
        return;
    }
    igRender();

    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(ui->window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.10f, 0.10f, 0.12f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
    glfwSwapBuffers(ui->window);
}

void ui_backend_shutdown(UiBackend *ui) {
    if (!ui) {
        return;
    }
    if (ui->window) {
        if (ui->imgui_ready) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            plot_destroy_context();
            igDestroyContext(NULL);
            ui->imgui_ready = false;
        }
        glfwDestroyWindow(ui->window);
        ui->window = NULL;
        glfwTerminate();
    }
}
