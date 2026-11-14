#ifndef APP_H
#define APP_H

#include <stdbool.h>

#include "alphavantage/alphavantage.h"

typedef struct GLFWwindow GLFWwindow;

typedef struct {
    AVClient client;
    bool ready;
} AVState;

typedef struct {
    float *prices;
    int price_count;
    float price_min;
    float price_max;
    char ticker[16];
    /* OHLC data for candlestick plotting (chronological order). */
    double *xs;
    double *opens;
    double *highs;
    double *lows;
    double *closes;
    int ohlc_count;
} ChartState;

typedef struct {
    GLFWwindow *window;
    AVState av;
    ChartState chart;
} AppState;

bool app_init(AppState *app);
void app_run(AppState *app);
void app_shutdown(AppState *app);

#endif /* APP_H */
