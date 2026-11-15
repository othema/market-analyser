#include "plot_c.h"

#include "imgui.h"
#include "implot.h"

// Creates the global ImPlot context.
// Must be called after the ImGui context is created. Returns nothing.
void plot_create_context(void) {
    ImPlot::CreateContext();
}

// Destroys the current ImPlot context.
// Should be called before the ImGui context is destroyed. Returns nothing.
void plot_destroy_context(void) {
    ImPlot::DestroyContext(nullptr);
}

// Begins a 2D ImPlot plot with the given title and size.
// If this returns true, plot_end() must be called.
// Returns true if the plot is visible and ready for drawing, otherwise false.
bool plot_begin(const char *title_id, PlotVec2 size, int flags) {
    ImVec2 plot_size(size.x, size.y);
    return ImPlot::BeginPlot(title_id ? title_id : "", plot_size, (ImPlotFlags)flags);
}

// Ends the current ImPlot plot.
// Must only be called when plot_begin() returned true. Returns nothing.
void plot_end(void) {
    ImPlot::EndPlot();
}

// Sets up X and Y axes labels and flags for the current plot.
// Returns nothing.
void plot_setup_axes(const char *x_label, const char *y_label, int x_flags, int y_flags) {
    ImPlot::SetupAxes(x_label, y_label, (ImPlotAxisFlags)x_flags, (ImPlotAxisFlags)y_flags);
}

// Sets the scale for a plot axis.
// axis: 0 = X1, 1 = X2, 2 = X3, 3 = Y1, 4 = Y2, 5 = Y3.
// scale: ImPlotScale value (for example 1 = ImPlotScale_Time in ImPlot v0.17).
// Returns nothing.
void plot_setup_axis_scale(int axis, int scale) {
    ImPlot::SetupAxisScale((ImAxis)axis, (ImPlotScale)scale);
}

// Sets custom tick positions and labels for an axis.
// labels may be NULL to keep default labels for the provided tick positions.
// Returns nothing.
void plot_setup_axis_ticks(int axis, const double *values, int n_ticks, const char *const *labels) {
    ImPlot::SetupAxisTicks((ImAxis)axis, values, n_ticks, labels, false);
}

// Sets axis limits for the current plot on X1 and Y1 axes.
// cond controls when limits are applied (for example ImPlotCond values).
// Returns nothing.
void plot_setup_axes_limits(double x_min, double x_max, double y_min, double y_max, int cond) {
    ImPlot::SetupAxesLimits(x_min, x_max, y_min, y_max, (ImPlotCond)cond);
}

// Returns the ImDrawList used by the current ImPlot plot for custom drawing.
// Returns an opaque pointer to the active draw list, or NULL if unavailable.
void *plot_get_draw_list(void) {
    return (void *)ImPlot::GetPlotDrawList();
}

// Converts plot-space coordinates (x, y) to pixel-space coordinates for X1/Y1 axes.
// Returns the pixel position as a PlotVec2.
PlotVec2 plot_to_pixels(double x, double y) {
    ImVec2 pixel = ImPlot::PlotToPixels(x, y, ImAxis_X1, ImAxis_Y1);
    PlotVec2 result = {pixel.x, pixel.y};
    return result;
}
