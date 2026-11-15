#ifndef UI_BACKEND_H
#define UI_BACKEND_H

#include <stdbool.h>

typedef struct GLFWwindow GLFWwindow;

typedef struct {
    GLFWwindow *window;
    bool imgui_ready;
} UiBackend;

bool ui_backend_init(UiBackend *ui, const char *title, int width, int height);
bool ui_backend_should_close(const UiBackend *ui);
void ui_backend_begin_frame(UiBackend *ui);
void ui_backend_end_frame(UiBackend *ui);
void ui_backend_shutdown(UiBackend *ui);

#endif // UI_BACKEND_H
