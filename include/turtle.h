#ifndef TURTLE_H
#define TURTLE_H

#include <stdint.h>
#include <stdbool.h>

#define TURTLE_RGBA(r, g, b, a) \
    (((uint32_t)(r) << 24) | ((uint32_t)(g) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(a))

/* Standard 8 Primary/Secondary Colors */
#define TURTLE_BLACK   TURTLE_RGBA(0,   0,   0,   255)
#define TURTLE_RED     TURTLE_RGBA(255, 0,   0,   255)
#define TURTLE_GREEN   TURTLE_RGBA(0,   255, 0,   255)
#define TURTLE_BLUE    TURTLE_RGBA(0,   0,   255, 255)
#define TURTLE_YELLOW  TURTLE_RGBA(255, 255, 0,   255)
#define TURTLE_MAGENTA TURTLE_RGBA(255, 0,   255, 255)
#define TURTLE_CYAN    TURTLE_RGBA(0,   255, 255, 255)
#define TURTLE_WHITE   TURTLE_RGBA(255, 255, 255, 255)

/* Lifecycle & Canvas Control
 * Note: High-level programs initialize via the function-like macro turtle_begin(context),
 * which is provided by the corresponding backend header (e.g. turtle_backend_raylib.h
 * or turtle_backend_canvas_wasi.h). The macro calls the backend-specific initializer
 * to guarantee link-time backend resolution, followed by turtle_begin_core().
 */
bool turtle_begin_core(void);
void turtle_end(void);
void turtle_clear(uint32_t color);
void turtle_home(void);
void turtle_delay(double ms);         /* Pause execution & flush canvas for progressive drawing */

/* Movement & Orientation */
void turtle_forward(double steps);   /* Positive = forward, Negative = backward */
void turtle_turn(double deg);        /* Math convention: CCW positive */
void turtle_turn_left(double deg);   /* Counter-clockwise */
void turtle_turn_right(double deg);  /* Clockwise */

/* Pen State Management */
void turtle_pen_down(void);
void turtle_pen_up(void);
int  turtle_is_pen_down(void);
void turtle_pen_stroke(double stroke);
void turtle_pen_color(uint32_t color);
void turtle_pen(double stroke, uint32_t color, int is_down);

/* Cursor Visibility */
void turtle_set_visible(bool visible);
bool turtle_is_visible(void);

/* State Inspection (Useful for students implementing turtle_goto) */
double turtle_get_x(void);
double turtle_get_y(void);
double turtle_get_heading(void);

#endif /* TURTLE_H */

