#ifndef PLOT_C_H
#define PLOT_C_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PlotVec2 {
    float x;
    float y;
} PlotVec2;

/* Creates the global ImPlot context. Must be called after ImGui context creation.
 * Returns nothing. */
void plot_create_context(void);

/* Destroys the global ImPlot context. Destroys the current context.
 * Should be called before ImGui context destruction. Returns nothing. */
void plot_destroy_context(void);

/* Begins a 2D ImPlot plot with the given title and size. If it returns true,
 * plot_end() must be called. size.x/size.y <= 0 use default/auto sizing.
 * Returns true if the plot is visible and ready for plotting, otherwise false. */
bool plot_begin(const char *title_id, PlotVec2 size, int flags);

/* Ends the current ImPlot plot. Must only be called if plot_begin() returned true.
 * Returns nothing. */
void plot_end(void);

/* Sets up the X and Y axes labels and flags for the current plot.
 * Returns nothing. */
void plot_setup_axes(const char *x_label, const char *y_label, int x_flags, int y_flags);

/* Sets the axis limits for the current plot on X1 and Y1 axes.
 * cond controls when limits are applied (e.g. 2 = ImPlotCond_Once).
 * Returns nothing. */
void plot_setup_axes_limits(double x_min, double x_max, double y_min, double y_max, int cond);

/* Returns the ImDrawList used by the current ImPlot plot for custom drawing.
 * Returns an opaque pointer to the active plot draw list, or NULL if unavailable. */
void *plot_get_draw_list(void);

/* Converts plot-space coordinates (x, y) to pixel-space coordinates using the
 * current plot's X1/Y1 axes.
 * Returns the pixel position as a PlotVec2. */
PlotVec2 plot_to_pixels(double x, double y);

#ifdef __cplusplus
}
#endif

#endif /* PLOT_C_H */
