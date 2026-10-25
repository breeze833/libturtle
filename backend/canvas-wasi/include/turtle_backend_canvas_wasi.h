#ifndef TURTLE_BACKEND_CANVAS_WASI_H
#define TURTLE_BACKEND_CANVAS_WASI_H

typedef struct {
    int width;
    int height;
    const char *title;
} turtle_canvas_wasi_context_t;

/* Alias for backwards compatibility */
typedef turtle_canvas_wasi_context_t turtle_htmlcanvas_context_t;

#endif /* TURTLE_BACKEND_CANVAS_WASI_H */
