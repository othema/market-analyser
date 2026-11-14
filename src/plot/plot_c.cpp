#include "plot_c.h"

#include "imgui.h"
#include "implot.h"

/* Creates the global ImPlot context. Must be called after ImGui context creation.
 * Returns nothing. */
void plot_create_context(void) {
    ImPlot::CreateContext();
}

/* Destroys the global ImPlot context. Destroys the current context.
 * Should be called before ImGui context destruction. Returns nothing. */
void plot_destroy_context(void) {
    ImPlot::DestroyContext(nullptr);
}

/* Begins a 2D ImPlot plot with the given title and size. If it returns true,
 * plot_end() must be called. Returns true if the plot is visible/ready, else false. */
bool plot_begin(const char *title_id, PlotVec2 size, int flags) {
    ImVec2 s(size.x, size.y);
    return ImPlot::BeginPlot(title_id ? title_id : "", s, (ImPlotFlags)flags);
}

/* Ends the current ImPlot plot. Must only be called if plot_begin() returned true.
 * Returns nothing. */
void plot_end(void) {
    ImPlot::EndPlot();
}

/* Sets up the X and Y axes labels and flags for the current plot.
 * Returns nothing. */
void plot_setup_axes(const char *x_label, const char *y_label, int x_flags, int y_flags) {
    ImPlot::SetupAxes(x_label, y_label, (ImPlotAxisFlags)x_flags, (ImPlotAxisFlags)y_flags);
}

/* Sets the axis limits for the current plot on X1 and Y1 axes.
 * cond controls when limits are applied (e.g. 2 = ImPlotCond_Once).
 * Returns nothing. */
void plot_setup_axes_limits(double x_min, double x_max, double y_min, double y_max, int cond) {
    ImPlot::SetupAxesLimits(x_min, x_max, y_min, y_max, (ImPlotCond)cond);
}

/* Returns the ImDrawList used by the current ImPlot plot for custom drawing.
 * Returns an opaque pointer to the active plot draw list, or NULL if unavailable. */
void *plot_get_draw_list(void) {
    return (void *)ImPlot::GetPlotDrawList();
}

/* Converts plot-space coordinates (x, y) to pixel-space coordinates using the
 * current plot's X1/Y1 axes. Returns the pixel position as a PlotVec2. */
PlotVec2 plot_to_pixels(double x, double y) {
    ImVec2 p = ImPlot::PlotToPixels(x, y, ImAxis_X1, ImAxis_Y1);
    PlotVec2 out = {p.x, p.y};
    return out;
}
