#include "turtle.h"
#include "turtle_backend_canvas_wasi.h"

int main(void) {
    turtle_canvas_wasi_context_t ctx = {
        .width = 800,
        .height = 600,
        .title = "libturtle - canvas-wasi Demo"
    };

    if (!turtle_begin(&ctx)) {
        return 1;
    }

    turtle_clear(TURTLE_WHITE);

    /* Draw a vibrant star spiral pattern */
    const uint32_t colors[] = {
        TURTLE_RED,
        TURTLE_YELLOW,
        TURTLE_GREEN,
        TURTLE_CYAN,
        TURTLE_BLUE,
        TURTLE_MAGENTA
    };
    const int num_colors = sizeof(colors) / sizeof(colors[0]);

    turtle_pen_stroke(2.5);

    for (int i = 0; i < 60; i++) {
        turtle_pen_color(colors[i % num_colors]);
        turtle_forward(i * 4.0);
        turtle_turn_right(144.0);
        turtle_delay(35.0); /* Progressive animation: 35ms per segment */
    }

    turtle_end();
    return 0;
}
