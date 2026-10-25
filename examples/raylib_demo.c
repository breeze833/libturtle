#include "turtle.h"
#include "turtle_backend_raylib.h"

int main(void) {
    turtle_raylib_context_t ctx = {
        .width = 800,
        .height = 600,
        .title = "libturtle - Raylib Demo",
        .target_fps = 60
    };

    if (!turtle_begin(&ctx)) {
        return 1;
    }

    turtle_clear(TURTLE_WHITE);

    /* Draw a colorful geometric rosette */
    const uint32_t colors[] = {
        TURTLE_RED,
        TURTLE_GREEN,
        TURTLE_BLUE,
        TURTLE_YELLOW,
        TURTLE_MAGENTA,
        TURTLE_CYAN
    };
    const int num_colors = sizeof(colors) / sizeof(colors[0]);

    turtle_pen_stroke(2.0);

    for (int i = 0; i < 72; i++) {
        turtle_pen_color(colors[i % num_colors]);
        turtle_forward(i * 3.5);
        turtle_turn_left(59.0);
        turtle_delay(25.0); /* Progressive animation: 25ms per segment */
    }

    turtle_end();
    return 0;
}
