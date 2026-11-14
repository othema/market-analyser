#define _POSIX_C_SOURCE 200809L

#include "app/app.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Loads KEY=VALUE pairs from a .env-style file into the process environment.
// Blank lines and lines starting with '#' are ignored.
// Existing environment variables are overwritten. Returns nothing.
void load_dotenv(const char *path) {
    if (!path) {
        return;
    }

    FILE *file = fopen(path, "r");
    if (!file) {
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        char *cursor = line;
        while (isspace((unsigned char)*cursor)) {
            cursor++;
        }
        if (*cursor == '\0' || *cursor == '#') {
            continue;
        }

        char *equals = strchr(cursor, '=');
        if (!equals) {
            continue;
        }

        *equals = '\0';
        char *key = cursor;
        char *value = equals + 1;

        // Trim trailing whitespace from the key.
        char *key_end = key + strlen(key);
        while (key_end > key && isspace((unsigned char)key_end[-1])) {
            *--key_end = '\0';
        }

        // Trim trailing newline/whitespace from the value.
        size_t value_length = strcspn(value, "\r\n");
        value[value_length] = '\0';

        if (key[0] == '\0') {
            continue;
        }

        // Overwrite existing environment variables with values from the file.
        setenv(key, value, 1);
    }

    fclose(file);
}

// Application entry point.
// Loads environment configuration, initializes and runs the UI, then shuts down.
// Returns 0 on success, or 1 on initialization failure.
int main(void) {
    load_dotenv(".env");

    AppState app = {0};
    if (!app_init(&app)) {
        fprintf(stderr, "Fatal error: failed to initialize application\n");
        return 1;
    }

    app_run(&app);
    app_shutdown(&app);

    return 0;
}
