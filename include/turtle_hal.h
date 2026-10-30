#ifndef TURTLE_HAL_H
#define TURTLE_HAL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Low-level Hardware Abstraction Layer for Turtle Graphics.
 * Implemented independently by libturtle_raylib and libturtle_canvas-wasi.
 * Coordinates passed to HAL are in physical screen space (origin at top-left).
 */

/* Initialize the graphics context/window */
bool hal_turtle_init(void *context);

/* Query current viewport/canvas dimensions */
void hal_turtle_get_canvas_size(double *out_width, double *out_height);

/* Finalize, flush buffer, or present frame */
void hal_turtle_present(void);

/* Flush canvas and pause execution for specified milliseconds */
void hal_turtle_delay(double ms);

/* Update turtle cursor position (screen coordinates), orientation, and visibility */
void hal_turtle_set_cursor(double screen_x, double screen_y, double heading_deg, bool visible);

/* Release resources / close window */
void hal_turtle_destroy(void);

/* Clear viewport with background color (RGBA8888) */
void hal_turtle_clear(uint32_t bg_color);

/* Draw a line segment between two screen points */
void hal_turtle_draw_line(double x1, double y1, 
                          double x2, double y2, 
                          double stroke_width, 
                          uint32_t color);

#endif /* TURTLE_HAL_H */

