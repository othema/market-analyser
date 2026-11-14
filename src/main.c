#define _POSIX_C_SOURCE 200809L  // setenv() is a POSIX function and not standard C.

#include "app/app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void load_dotenv(const char *path);

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

void load_dotenv(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '#') continue;          /* blank/comment */

        char *eq = strchr(p, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = p;
        char *val = eq + 1;

        /* trim end of key and newline/whitespace from value */
        char *end = key + strlen(key);
        while (end > key && isspace((unsigned char)end[-1])) *--end = '\0';
        size_t vlen = strcspn(val, "\r\n");
        val[vlen] = '\0';

        /* (optional) strip surrounding quotes from val here */
        setenv(key, val, 1); /* overwrite existing; use 0 to keep existing */
    }
    fclose(f);
}
