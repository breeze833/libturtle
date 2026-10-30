#include "turtle.h"
#include "turtle_hal.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    double x;           /* Math coordinate X (center 0, right positive) */
    double y;           /* Math coordinate Y (center 0, up positive) */
    double heading_deg; /* Standard math angle in degrees (90 deg = North) */
    double stroke;
    uint32_t color;
    bool is_down;
    bool is_visible;
    double canvas_w;
    double canvas_h;
} turtle_state_t;

static turtle_state_t g_turtle;

static inline double to_screen_x(double math_x) {
    return (g_turtle.canvas_w / 2.0) + math_x;
}

static inline double to_screen_y(double math_y) {
    return (g_turtle.canvas_h / 2.0) - math_y;
}

static inline void sync_cursor(void) {
    hal_turtle_set_cursor(
        to_screen_x(g_turtle.x),
        to_screen_y(g_turtle.y),
        g_turtle.heading_deg,
        g_turtle.is_visible
    );
}

bool turtle_begin_core(void) {
    hal_turtle_get_canvas_size(&g_turtle.canvas_w, &g_turtle.canvas_h);
    g_turtle.is_visible = true;
    turtle_home();
    g_turtle.stroke = 2.0;
    g_turtle.color = TURTLE_BLACK;
    g_turtle.is_down = true;
    sync_cursor();
    return true;
}

void turtle_end(void) {
    hal_turtle_present();
    hal_turtle_destroy();
}

void turtle_home(void) {
    g_turtle.x = 0.0;
    g_turtle.y = 0.0;
    g_turtle.heading_deg = 90.0; /* Default orientation: North */
    sync_cursor();
}

void turtle_clear(uint32_t color) {
    hal_turtle_clear(color);
}

void turtle_delay(double ms) {
    hal_turtle_delay(ms);
}

void turtle_forward(double steps) {
    double rad = g_turtle.heading_deg * (M_PI / 180.0);
    double next_x = g_turtle.x + steps * cos(rad);
    double next_y = g_turtle.y + steps * sin(rad);

    if (g_turtle.is_down) {
        hal_turtle_draw_line(
            to_screen_x(g_turtle.x), to_screen_y(g_turtle.y),
            to_screen_x(next_x),     to_screen_y(next_y),
            g_turtle.stroke,
            g_turtle.color
        );
    }

    g_turtle.x = next_x;
    g_turtle.y = next_y;
    sync_cursor();
}

void turtle_turn(double deg) {
    g_turtle.heading_deg += deg;
    /* Normalize heading into [0, 360) */
    g_turtle.heading_deg = fmod(g_turtle.heading_deg, 360.0);
    if (g_turtle.heading_deg < 0.0) {
        g_turtle.heading_deg += 360.0;
    }
    sync_cursor();
}

void turtle_turn_left(double deg) {
    turtle_turn(deg);
}

void turtle_turn_right(double deg) {
    turtle_turn(-deg);
}

void turtle_set_visible(bool visible) {
    g_turtle.is_visible = visible;
    sync_cursor();
}

bool turtle_is_visible(void) {
    return g_turtle.is_visible;
}

void turtle_pen_down(void) { g_turtle.is_down = true; }
void turtle_pen_up(void)   { g_turtle.is_down = false; }
int  turtle_is_pen_down(void) { return g_turtle.is_down ? 1 : 0; }
void turtle_pen_stroke(double stroke) { g_turtle.stroke = stroke; }
void turtle_pen_color(uint32_t color) { g_turtle.color = color; }

void turtle_pen(double stroke, uint32_t color, int is_down) {
    g_turtle.stroke = stroke;
    g_turtle.color = color;
    g_turtle.is_down = (is_down != 0);
}

double turtle_get_x(void)       { return g_turtle.x; }
double turtle_get_y(void)       { return g_turtle.y; }
double turtle_get_heading(void) { return g_turtle.heading_deg; }

