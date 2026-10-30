#ifndef TURTLE_BACKEND_CANVAS_WASI_H
#define TURTLE_BACKEND_CANVAS_WASI_H

#include <stdbool.h>

typedef struct {
    int width;
    int height;
    const char *title;
} turtle_canvas_wasi_context_t;

/**
 * Backend-specific initialization function for Canvas WASI.
 * Calling this directly or via turtle_begin() guarantees that the program will fail
 * to link if linked against a different backend (e.g. raylib).
 */
bool canvas_wasi_turtle_init(void *context);

/**
 * Convenience macro to initialize the Canvas WASI backend and turtle state machine.
 */
#define turtle_begin(ctx) (canvas_wasi_turtle_init(ctx) && turtle_begin_core())

#endif /* TURTLE_BACKEND_CANVAS_WASI_H */
