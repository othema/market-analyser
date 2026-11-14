#ifndef STOCK_SELECTOR_H
#define STOCK_SELECTOR_H

#include "app/app.h"

// Renders stock selector controls and handles fetch requests from the UI.
// Intended to be called inside an existing ImGui window/child region.
// Returns nothing.
void stock_selector_render(AppState *app, char *ticker, int ticker_size);

#endif // STOCK_SELECTOR_H

