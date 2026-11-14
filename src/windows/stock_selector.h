#ifndef STOCK_SELECTOR_H
#define STOCK_SELECTOR_H

#include "app/app.h"

// Renders the stock selector window and handles fetch requests from the UI.
// Returns nothing.
void stock_selector_render(AppState *app, char *ticker, int ticker_size);

#endif // STOCK_SELECTOR_H

