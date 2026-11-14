#ifndef PLOT_C_H
#define PLOT_C_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Simple 2D vector used by the C plot wrapper API.
typedef struct PlotVec2 {
    float x;
    float y;
} PlotVec2;

// Creates the global ImPlot context.
// Must be called after the ImGui context is created. Returns nothing.
void plot_create_context(void);

// Destroys the current ImPlot context.
// Should be called before the ImGui context is destroyed. Returns nothing.
void plot_destroy_context(void);

// Begins a 2D ImPlot plot with the given title and size.
// If this returns true, plot_end() must be called.
// size.x/size.y <= 0 use default/auto sizing.
// Returns true if the plot is visible and ready for drawing, otherwise false.
bool plot_begin(const char *title_id, PlotVec2 size, int flags);

// Ends the current ImPlot plot.
// Must only be called when plot_begin() returned true. Returns nothing.
void plot_end(void);

// Sets up X and Y axes labels and flags for the current plot.
// Returns nothing.
void plot_setup_axes(const char *x_label, const char *y_label, int x_flags, int y_flags);

// Sets axis limits for the current plot on X1 and Y1 axes.
// cond controls when limits are applied (for example ImPlotCond values).
// Returns nothing.
void plot_setup_axes_limits(double x_min, double x_max, double y_min, double y_max, int cond);

// Returns the ImDrawList used by the current ImPlot plot for custom drawing.
// Returns an opaque pointer to the active draw list, or NULL if unavailable.
void *plot_get_draw_list(void);

// Converts plot-space coordinates (x, y) to pixel-space coordinates for X1/Y1 axes.
// Returns the pixel position as a PlotVec2.
PlotVec2 plot_to_pixels(double x, double y);

#ifdef __cplusplus
}
#endif

#endif // PLOT_C_H
