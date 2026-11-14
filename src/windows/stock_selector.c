#include "windows/stock_selector.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include "alphavantage/alphavantage.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void stock_selector_render(AppState *app, char *ticker, int ticker_size) {
    igBegin("Stock Selector", NULL, 0);

    igInputTextWithHint("Ticker", "AAPL", ticker, ticker_size, ImGuiInputTextFlags_None, NULL, NULL);
    if (igButton("View data", (ImVec2){0, 0})) {
        if (!ticker || ticker[0] == '\0') {  // Has user entered a ticker?
            printf("No ticker entered.\n");
            fflush(stdout);
        } else if (!app || !app->av.ready) {  // Has the AV client been created?
            printf("Alpha Vantage client is not initialized.\n");
            fflush(stdout);
        } else {
            
            AVTimeSeries series = {0};
            AVStatus status = av_get_time_series_daily(&app->av.client, ticker, &series);
            if (status != AVOk) {
                printf("Failed to fetch stock data for %s: %s\n", ticker, av_status_str(status));
            } else {
                printf("Stock data for %s (%zu points):\n", ticker, series.count);
                printf("%-12s %10s %10s %10s %10s %12s\n", "date", "open", "high", "low", "close", "volume");
                for (size_t i = 0; i < series.count; i++) {
                    AVTimeSeriesPoint *p = &series.points[i];
                    printf("%-12s %10.2f %10.2f %10.2f %10.2f %12lld\n",
                           p->date ? p->date : "?",
                           p->open, p->high, p->low, p->close,
                           (long long)p->volume);
                }

                if (series.count > 0) {
                    float *prices = (float *)malloc(series.count * sizeof(float));
                    double *xs = (double *)malloc(series.count * sizeof(double));
                    double *opens = (double *)malloc(series.count * sizeof(double));
                    double *highs = (double *)malloc(series.count * sizeof(double));
                    double *lows = (double *)malloc(series.count * sizeof(double));
                    double *closes = (double *)malloc(series.count * sizeof(double));
                    if (prices && xs && opens && highs && lows && closes) {
                        float min_v = FLT_MAX;
                        float max_v = -FLT_MAX;
                        /* API returns most recent first; reverse to chronological for plotting. */
                        for (size_t i = 0; i < series.count; i++) {
                            size_t src = series.count - 1 - i;
                            AVTimeSeriesPoint *p = &series.points[src];
                            float v = (float)p->close;
                            prices[i] = v;
                            xs[i] = (double)i; /* simple index-based x axis */
                            opens[i] = p->open;
                            highs[i] = p->high;
                            lows[i] = p->low;
                            closes[i] = p->close;
                            if (v < min_v) min_v = v;
                            if (v > max_v) max_v = v;
                        }
                        if (min_v == max_v) {
                            min_v -= 1.0f;
                            max_v += 1.0f;
                        }
                        free(app->chart.prices);
                        app->chart.prices = prices;
                        app->chart.price_count = (int)series.count;
                        app->chart.price_min = min_v;
                        app->chart.price_max = max_v;
                        strncpy(app->chart.ticker, ticker, sizeof(app->chart.ticker) - 1);
                        app->chart.ticker[sizeof(app->chart.ticker) - 1] = '\0';

                        free(app->chart.xs);
                        free(app->chart.opens);
                        free(app->chart.highs);
                        free(app->chart.lows);
                        free(app->chart.closes);
                        app->chart.xs = xs;
                        app->chart.opens = opens;
                        app->chart.highs = highs;
                        app->chart.lows = lows;
                        app->chart.closes = closes;
                        app->chart.ohlc_count = (int)series.count;
                    } else {
                        free(prices);
                        free(xs);
                        free(opens);
                        free(highs);
                        free(lows);
                        free(closes);
                        printf("Failed to allocate memory for plot data.\n");
                    }
                }
            }
            fflush(stdout);

            av_time_series_free(&series);
        }
    }

    igEnd();
}
