#include "plot_c.h"

#include "imgui.h"
#include "implot.h"

// Creates the global ImPlot context.
// Must be called after the ImGui context is created.
void plot_create_context(void) {
    ImPlot::CreateContext();
}

// Destroys the current ImPlot context.
// Should be called before the ImGui context is destroyed.
void plot_destroy_context(void) {
    ImPlot::DestroyContext(nullptr);
}

// Begins a 2D ImPlot plot with the given title and size.
// If this returns true, plot_end() must be called.
bool plot_begin(const char *title_id, PlotVec2 size, int flags) {
    ImVec2 plot_size(size.x, size.y);
    return ImPlot::BeginPlot(title_id ? title_id : "", plot_size, (ImPlotFlags)flags);
}

// Ends the current ImPlot plot.
// Must only be called when plot_begin() returned true.
void plot_end(void) {
    ImPlot::EndPlot();
}

// Sets up X and Y axes labels and flags for the current plot.
void plot_setup_axes(const char *x_label, const char *y_label, int x_flags, int y_flags) {
    ImPlot::SetupAxes(x_label, y_label, (ImPlotAxisFlags)x_flags, (ImPlotAxisFlags)y_flags);
}

// Sets the scale for a plot axis.
// axis: 0 = X1, 1 = X2, 2 = X3, 3 = Y1, 4 = Y2, 5 = Y3.
// scale: ImPlotScale value (for example 1 = ImPlotScale_Time in ImPlot v0.17).
void plot_setup_axis_scale(int axis, int scale) {
    ImPlot::SetupAxisScale((ImAxis)axis, (ImPlotScale)scale);
}

// Sets custom tick positions and labels for an axis.
// labels may be NULL to keep default labels for the provided tick positions.
void plot_setup_axis_ticks(int axis, const double *values, int n_ticks, const char *const *labels) {
    ImPlot::SetupAxisTicks((ImAxis)axis, values, n_ticks, labels, false);
}

// Sets axis limits for the current plot on X1 and Y1 axes.
// cond controls when limits are applied (for example ImPlotCond values).
void plot_setup_axes_limits(double x_min, double x_max, double y_min, double y_max, int cond) {
    ImPlot::SetupAxesLimits(x_min, x_max, y_min, y_max, (ImPlotCond)cond);
}

void *plot_get_draw_list(void) {
    return (void *)ImPlot::GetPlotDrawList();
}

// Converts plot-space coordinates (x, y) to pixel-space coordinates for X1/Y1 axes.
PlotVec2 plot_to_pixels(double x, double y) {
    ImVec2 pixel = ImPlot::PlotToPixels(x, y, ImAxis_X1, ImAxis_Y1);
    PlotVec2 result = {pixel.x, pixel.y};
    return result;
}

// Plots a line series using X1/Y1 axes.
// flags can be 0 for default styling.
void plot_plot_line(const char *label_id, const double *xs, const double *ys, int count, int flags) {
    if (!xs || !ys || count <= 0) {
        return;
    }
    ImPlot::PlotLine(label_id ? label_id : "", xs, ys, count, (ImPlotLineFlags)flags, 0, sizeof(double));
}

// Sets the color/style for the next plotted line item.
void plot_set_next_line_style(float r, float g, float b, float a, float weight) {
    ImPlot::SetNextLineStyle(ImVec4(r, g, b, a), weight);
}

bool plot_is_hovered(void) {
    return ImPlot::IsPlotHovered();
}

// axis constants: 0 = X1, 3 = Y1 (ImAxis values), use -1 for IMPLOT_AUTO.
PlotPointD plot_get_mouse_pos(int x_axis, int y_axis) {
    ImPlotPoint p = ImPlot::GetPlotMousePos((ImAxis)x_axis, (ImAxis)y_axis);
    PlotPointD out = {p.x, p.y};
    return out;
}

// axis constants: 0 = X1, 3 = Y1 (ImAxis values), use -1 for IMPLOT_AUTO.
PlotRectD plot_get_plot_limits(int x_axis, int y_axis) {
    ImPlotRect r = ImPlot::GetPlotLimits((ImAxis)x_axis, (ImAxis)y_axis);
    PlotRectD out = {r.X.Min, r.X.Max, r.Y.Min, r.Y.Max};
    return out;
}
