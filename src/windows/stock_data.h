#ifndef STOCK_DATA_H
#define STOCK_DATA_H

#include "app/app.h"

// Renders stock data and the candlestick chart for the current chart state.
// Intended to be called inside an existing ImGui window/child region.
// Returns nothing.
void stock_data_render(AppState *app);

#endif // STOCK_DATA_H

