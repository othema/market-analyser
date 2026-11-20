#include "app/app.h"

#include <stdio.h>

// Application entry point.
// Initializes and runs the UI, then shuts down.
int main(void) {
    AppState app = {0};
    if (!app_init(&app)) {
        fprintf(stderr, "Fatal error: failed to initialize application\n");
        return 1;
    }

    app_run(&app);
    app_shutdown(&app);

    return 0;
}
