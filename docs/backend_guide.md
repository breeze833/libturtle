# libturtle Backend Developer Guide & HAL Specification

This guide is intended for engine developers, contributors, and graphics engineers who are implementing a new rendering backend (such as SDL2, Cairo, WebGL, or an SVG exporter) or maintaining existing backends (`raylib` and `canvas-wasi`) for `libturtle`.

---

## 1. Architectural Overview & Design Philosophy

`libturtle` enforces a strict separation of concerns across a multi-tier decoupled architecture:

```
┌─────────────────────────────────────────────────────────────────┐
│                    User Application Layer                       │
│  - Calls turtle_forward(), turtle_turn(), turtle_pen(), etc.    │
│  - Selects backend: #include "turtle_backend_<name>.h"          │
│  - Configures context: turtle_<name>_context_t ctx = { ... };   │
└────────────────────────────────┬────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│              Core Engine (src/turtle.c)                         │
│  - Single source of truth for turtle state                      │
│  - State: x, y, heading_deg, stroke, color, is_down, visible    │
│  - Cartesian math & standard trigonometric calculations         │
│  - Converts math coordinates into physical screen coordinates   │
└────────────────────────────────┬────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│      Hardware Abstraction Layer (include/turtle_hal.h)         │
│  - Pure C HAL contract with 8 functions                         │
│  - Operates exclusively in physical screen space (0,0 = top-left│
│  - Strictly decoupled from turtle orientation & trigonometry    │
└────────────────────────────────┬────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│       Backend Implementation (backend/<name>/hal_<name>.c)       │
│  - Window lifecycle, framebuffers, textures, and canvas surfaces│
│  - Vector rasterization / persistent stroke buffer              │
│  - Non-destructive cursor visualization overlay                 │
│  - Frame pacing, progressive animation, and interactive loops   │
└─────────────────────────────────────────────────────────────────┘
```

### 1.1 State Machine Invariant
All turtle state (`x`, `y`, `heading_deg`, `stroke`, `color`, `is_down`, `is_visible`) lives strictly inside `src/turtle.c`.
- **Never expose backend-specific structures or types inside `include/turtle.h` or `include/turtle_hal.h`.**
- **Keep line drawing decoupled from turtle heading:** `hal_turtle_draw_line()` receives only physical screen coordinates `(x1, y1)` and `(x2, y2)`. Heading orientation is passed exclusively to `hal_turtle_set_cursor()`.

### 1.2 Coordinate Transformation
The core `src/turtle.c` translates mathematical Cartesian coordinates into physical screen coordinates:

- $x_{\text{screen}} = \frac{W_{\text{canvas}}}{2.0} + x_{\text{math}}$
- $y_{\text{screen}} = \frac{H_{\text{canvas}}}{2.0} - y_{\text{math}}$

| Property | Mathematical Space (`turtle.c`) | Physical Screen Space (`turtle_hal.h`) |
|---|---|---|
| **Origin $(0,0)$** | Center of canvas viewport | Top-left corner of canvas |
| **$+X$ Direction** | Right (East) | Right |
| **$+Y$ Direction** | Up (North) | Down |
| **Heading Angle** | Math standard: $90^\circ = \text{North}$, CCW positive | Forwarded directly to cursor renderer |

---

## 2. Link-Time Backend Resolution Pattern

One of the central design challenges of a modular C library is safety when initializing platform-dependent contexts.

### 2.1 The Problem
If `turtle_begin(void *context)` were exposed directly as a generic C function, a user could include `turtle_backend_raylib.h` (expecting a Raylib context), but link against `libturtle_canvas-wasi.a`. The C compiler would emit no warnings, but at runtime the WASI backend would cast the Raylib context struct to a WASI context, causing memory corruption or unexpected behavior.

### 2.2 The Solution
`libturtle` enforces **Link-Time Backend Resolution**:

1. **Unique Backend Initializer**: Each backend exports a uniquely named initializer function in its public backend header:
   - Raylib: `bool raylib_turtle_init(void *context);`
   - Canvas WASI: `bool canvas_wasi_turtle_init(void *context);`
2. **Platform-Agnostic Core Initializer**: `src/turtle.c` implements `turtle_begin_core(void)`.
3. **The `turtle_begin` Macro**: The backend header defines:
   ```c
   #define turtle_begin(ctx) (<backend>_turtle_init(ctx) && turtle_begin_core())
   ```

```
User Program includes "turtle_backend_raylib.h"
Calls: turtle_begin(&raylib_ctx)
        │
        ├──> Calls raylib_turtle_init(&raylib_ctx)   <── Must be resolved by libturtle_raylib.a!
        │    (If linked with libturtle_canvas-wasi.a, linker fails with: "undefined reference to raylib_turtle_init")
        │
        └──> Calls turtle_begin_core()               <── Resolved by src/turtle.c
```

> [!IMPORTANT]
> Do NOT expose a generic `turtle_begin()` symbol in `src/turtle.c`. Doing so breaks this link-time resolution safety guarantee.

---

## 3. Hardware Abstraction Layer Specification (`turtle_hal.h`)

Every backend must implement the following 8 C functions declared in `include/turtle_hal.h`.

```c
#ifndef TURTLE_HAL_H
#define TURTLE_HAL_H

#include <stdint.h>
#include <stdbool.h>

bool hal_turtle_init(void *context);
void hal_turtle_get_canvas_size(double *out_width, double *out_height);
void hal_turtle_present(void);
void hal_turtle_delay(double ms);
void hal_turtle_set_cursor(double screen_x, double screen_y, double heading_deg, bool visible);
void hal_turtle_destroy(void);
void hal_turtle_clear(uint32_t bg_color);
void hal_turtle_draw_line(double x1, double y1, double x2, double y2, double stroke_width, uint32_t color);

#endif /* TURTLE_HAL_H */
```

### 3.1 `hal_turtle_init`
```c
bool hal_turtle_init(void *context);
```
- **Purpose**: Creates the native window, initializes graphics contexts, allocates persistent render buffers, and sets initial canvas dimensions.
- **Parameters**: Pointer to backend-specific context struct (cast within the backend). If `NULL`, use sensible defaults (e.g. $800 \times 600$, 60 FPS).
- **Returns**: `true` on success, `false` on initialization failure.
- **Backend Initializer Binding**: Your backend's public initializer must delegate to this function:
  ```c
  bool <backend>_turtle_init(void *context) {
      return hal_turtle_init(context);
  }
  ```

### 3.2 `hal_turtle_get_canvas_size`
```c
void hal_turtle_get_canvas_size(double *out_width, double *out_height);
```
- **Purpose**: Populates the physical width and height of the drawing surface.
- **Parameters**: Pointers to `double`. The implementation must check for `NULL` before writing.
- **Called by**: `turtle_begin_core()` to establish coordinate bounds and calculate the center origin $(W/2, H/2)$.

### 3.3 `hal_turtle_present`
```c
void hal_turtle_present(void);
```
- **Purpose**: Concludes active drawing and enters an interactive window/event loop.
- **Requirements**:
  - Present the final drawing with the cursor overlay (if visible).
  - Run a message/event loop keeping the window responsive.
  - Listen for the `ESC` key (ASCII 27) and OS window close events.
  - Return control only after the user requests window close or presses `ESC`.

### 3.4 `hal_turtle_delay`
```c
void hal_turtle_delay(double ms);
```
- **Purpose**: Flushes intermediate drawing commands and pauses execution for `ms` milliseconds to support progressive animation.
- **Requirements**:
  - Present current drawing state so changes are immediately visible.
  - **Dynamic pacing**: Slice long delays into small sub-frame intervals (e.g. $\le 16.6\text{ ms}$ for 60 FPS) and poll input/events.
  - **Prompt termination**: If window close or `ESC` is detected during the delay, clean up resources and exit the process immediately (`exit(0)`). Do not keep running drawing commands after the user closed the window.

### 3.5 `hal_turtle_set_cursor`
```c
void hal_turtle_set_cursor(double screen_x, double screen_y, double heading_deg, bool visible);
```
- **Purpose**: Updates the cursor position (in physical screen coordinates), orientation, and visibility.
- **Parameters**:
  - `screen_x`, `screen_y`: Position in physical screen coordinates.
  - `heading_deg`: Turtle orientation in degrees ($90^\circ$ = North).
  - `visible`: `true` if cursor should be rendered, `false` if hidden.
- **Critical Requirement**: See Section 4.2 for non-destructive cursor rendering invariants.

### 3.6 `hal_turtle_destroy`
```c
void hal_turtle_destroy(void);
```
- **Purpose**: Frees all allocated graphics memory, destroys offscreen textures/surfaces, closes windows, and shuts down backend subsystems.

### 3.7 `hal_turtle_clear`
```c
void hal_turtle_clear(uint32_t bg_color);
```
- **Purpose**: Fills the persistent canvas buffer with `bg_color`.
- **Parameters**: Packed 32-bit `0xRRGGBBAA` color.

### 3.8 `hal_turtle_draw_line`
```c
void hal_turtle_draw_line(double x1, double y1, double x2, double y2, double stroke_width, uint32_t color);
```
- **Purpose**: Renders a vector line segment from `(x1, y1)` to `(x2, y2)` onto the persistent canvas buffer.
- **Parameters**: Screen coordinates, stroke width in pixels, and packed `0xRRGGBBAA` color.
- **Style Requirement**: Use rounded line caps and joints (`LineCap::Round`) when available in the graphics API to prevent gaps between consecutive segments.

---

## 4. Backend Implementation Invariants

When implementing a backend, you must respect the following core invariants:

### 4.1 Stroke Persistence Across Frames
In traditional game engines (like Raylib), the main framebuffer is cleared at the start of every frame (`BeginDrawing(); ClearBackground(); ...`).
**Turtle graphics requires persistent drawing**: lines drawn on frame $N$ must remain visible on frame $N + 1000$ without re-executing previous line commands.

- **Raylib Approach**: Create an offscreen `RenderTexture2D` during `hal_turtle_init()`. In `hal_turtle_draw_line()`, draw lines permanently into `RenderTexture2D` via `BeginTextureMode(target)`. In `present()`, render the `RenderTexture2D` to the window framebuffer.
- **Canvas WASI Approach**: The HTML5 2D `<canvas>` naturally retains drawn vector paths across frames. `canvas_wasi` transfers canvas frames using an intermediate clone buffer to preserve drawing state.

### 4.2 Non-Destructive Cursor Overlay
The stylized turtle indicator displays the current position and direction of the turtle. **The cursor must never corrupt or erase drawn lines on the canvas.**

There are two approved strategies:

1. **Display Framebuffer Overlay (Used by Raylib)**:
   - The persistent canvas (`RenderTexture2D`) contains only the user's drawings.
   - During `hal_turtle_present()` or `hal_turtle_delay()`, draw `RenderTexture2D` onto the window screen, and then draw the cursor directly onto the window screen (NOT onto the `RenderTexture2D`).
   - The canvas texture remains untouched.
2. **Snapshot and Restore (Used by Canvas WASI)**:
   - Before drawing the cursor on the canvas, take a pixel snapshot of the bounding box underneath the cursor via `canvas_get_image_data()`.
   - Render the cursor.
   - Before the next line segment is drawn (or before clearing), restore the original canvas pixels via `canvas_put_image_data()`.

### 4.3 Color Extraction
Colors are packed 32-bit unsigned integers formatted as `0xRRGGBBAA`. Define standard extraction macros:

```c
#define RGBA_R(c) (((c) >> 24) & 0xFF)
#define RGBA_G(c) (((c) >> 16) & 0xFF)
#define RGBA_B(c) (((c) >> 8)  & 0xFF)
#define RGBA_A(c) ((c)         & 0xFF)
```

### 4.4 Frame Pacing and Prompt Exit in `hal_turtle_delay()`
Students often run programs with delays in tight loops. If a student closes the window during an animation, the program must terminate promptly.

```c
void hal_turtle_delay(double ms) {
    if (ms <= 0.0) return;

    /* 1. Present intermediate canvas so user sees progress */
    // ... draw persistent canvas + cursor overlay ...

    /* 2. Pace in short frame slices (e.g. 16ms / 60 FPS) */
    double remaining = ms;
    while (remaining > 0.0) {
        if (platform_should_close() || platform_is_esc_pressed()) {
            hal_turtle_destroy();
            exit(0); /* Clean exit immediately */
        }
        
        double slice = (remaining > 16.0) ? 16.0 : remaining;
        platform_sleep_ms(slice);
        platform_poll_events();
        remaining -= slice;
    }
}
```

---

## 5. Step-by-Step Tutorial: Implementing a New Backend

Suppose you want to implement an SDL2 backend (`sdl2`). Here is the step-by-step procedure:

### Step 1: Create Backend Directory Structure
```bash
mkdir -p backend/sdl2/include
```

### Step 2: Define Context and Header (`backend/sdl2/include/turtle_backend_sdl2.h`)
```c
#ifndef TURTLE_BACKEND_SDL2_H
#define TURTLE_BACKEND_SDL2_H

#include <stdbool.h>

typedef struct {
    int width;
    int height;
    const char *title;
    int target_fps;
} turtle_sdl2_context_t;

/* Backend-specific initializer symbol */
bool sdl2_turtle_init(void *context);

/* Macro enforcing link-time backend resolution */
#define turtle_begin(ctx) (sdl2_turtle_init(ctx) && turtle_begin_core())

#endif /* TURTLE_BACKEND_SDL2_H */
```

### Step 3: Implement HAL Functions (`backend/sdl2/hal_sdl2.c`)
```c
#include "turtle_hal.h"
#include "turtle_backend_sdl2.h"
#include <SDL2/SDL.h>
#include <stdlib.h>

#define RGBA_R(c) (((c) >> 24) & 0xFF)
#define RGBA_G(c) (((c) >> 16) & 0xFF)
#define RGBA_B(c) (((c) >> 8)  & 0xFF)
#define RGBA_A(c) ((c)         & 0xFF)

static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture *g_canvas_texture = NULL;
static int g_width = 800;
static int g_height = 600;

bool sdl2_turtle_init(void *context) {
    return hal_turtle_init(context);
}

bool hal_turtle_init(void *context) {
    if (context != NULL) {
        turtle_sdl2_context_t *ctx = (turtle_sdl2_context_t *)context;
        if (ctx->width > 0) g_width = ctx->width;
        if (ctx->height > 0) g_height = ctx->height;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

    g_window = SDL_CreateWindow("Turtle Graphics",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        g_width, g_height, SDL_WINDOW_SHOWN);
    if (!g_window) return false;

    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    if (!g_renderer) return false;

    /* Create target texture for persistent drawing */
    g_canvas_texture = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        g_width, g_height);

    /* Initialize background to white */
    SDL_SetRenderTarget(g_renderer, g_canvas_texture);
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    SDL_RenderClear(g_renderer);
    SDL_SetRenderTarget(g_renderer, NULL);

    return true;
}

void hal_turtle_get_canvas_size(double *out_width, double *out_height) {
    if (out_width)  *out_width = (double)g_width;
    if (out_height) *out_height = (double)g_height;
}

void hal_turtle_clear(uint32_t bg_color) {
    SDL_SetRenderTarget(g_renderer, g_canvas_texture);
    SDL_SetRenderDrawColor(g_renderer, RGBA_R(bg_color), RGBA_G(bg_color), RGBA_B(bg_color), RGBA_A(bg_color));
    SDL_RenderClear(g_renderer);
    SDL_SetRenderTarget(g_renderer, NULL);
}

void hal_turtle_draw_line(double x1, double y1, double x2, double y2, double stroke_width, uint32_t color) {
    SDL_SetRenderTarget(g_renderer, g_canvas_texture);
    SDL_SetRenderDrawColor(g_renderer, RGBA_R(color), RGBA_G(color), RGBA_B(color), RGBA_A(color));
    SDL_RenderDrawLine(g_renderer, (int)x1, (int)y1, (int)x2, (int)y2);
    SDL_SetRenderTarget(g_renderer, NULL);
}

void hal_turtle_set_cursor(double screen_x, double screen_y, double heading_deg, bool visible) {
    /* Cache cursor state for rendering during present / delay */
}

void hal_turtle_present(void) {
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        /* Render persistent canvas */
        SDL_RenderCopy(g_renderer, g_canvas_texture, NULL, NULL);
        /* Render non-destructive cursor overlay */
        // ... draw turtle cursor ...
        SDL_RenderPresent(g_renderer);
        SDL_Delay(16);
    }
}

void hal_turtle_delay(double ms) {
    // Pace with sub-frame slicing and check for SDL_QUIT or ESC
}

void hal_turtle_destroy(void) {
    if (g_canvas_texture) SDL_DestroyTexture(g_canvas_texture);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);
    SDL_Quit();
}
```

### Step 4: Integrate with `Makefile`
Add conditional compilation blocks to the `Makefile` to compile `src/turtle.c` and `backend/sdl2/hal_sdl2.c` into `build/sdl2/libturtle_sdl2.a`.

---

## 6. Case Studies

### 6.1 Raylib Backend (`backend/raylib/hal_raylib.c`)
- Uses hardware-accelerated `RenderTexture2D` for persistent canvas graphics.
- Cursor is drawn on top of the screen framebuffer in `BeginDrawing()` / `EndDrawing()` after copying the render texture, leaving the canvas buffer pristine.
- `hal_turtle_delay()` computes remainder slices using `GetTime()` and `WaitTime()` while checking `WindowShouldClose()`.

### 6.2 Canvas WASI Backend (`backend/canvas-wasi/hal_canvas_wasi.c`)
- Communicates across the WebAssembly boundary with `canvas-wasi` via `<graphics_canvas.h>` and `<graphics.h>`.
- Preserves canvas state using retained 2D context paths.
- Uses snapshotting (`canvas_get_image_data`) and restoration (`canvas_put_image_data`) for the turtle cursor indicator.
- Dynamic header synchronization: The Makefile dynamically fetches `<graphics.h>` and `<graphics_canvas.h>` from upstream `breeze833/canvas-wasi` (with local directory override via `CANVAS_WASI_DIR`), keeping external dependencies gitignored.
