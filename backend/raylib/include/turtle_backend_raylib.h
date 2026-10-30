#ifndef TURTLE_BACKEND_RAYLIB_H
#define TURTLE_BACKEND_RAYLIB_H

#include <stdbool.h>

typedef struct {
    int width;
    int height;
    const char *title;
    int target_fps;
} turtle_raylib_context_t;

/**
 * Backend-specific initialization function for Raylib.
 * Calling this directly or via turtle_begin() guarantees that the program will fail
 * to link if linked against a different backend (e.g. canvas-wasi).
 */
bool raylib_turtle_init(void *context);

/**
 * Convenience macro to initialize the Raylib backend and turtle state machine.
 */
#define turtle_begin(ctx) (raylib_turtle_init(ctx) && turtle_begin_core())

#endif /* TURTLE_BACKEND_RAYLIB_H */
