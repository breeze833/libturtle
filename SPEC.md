# C Turtle Graphics Library Specification

## 1. Overview
A cross-platform, modular educational C turtle graphics library designed for undergraduate computer science pedagogy. The library decouples high-level geometric navigation logic from rasterization backends using a build-time Hardware Abstraction Layer (HAL).

Initial targets:
1. **Raylib Backend (`raylib`)**: Desktop native rendering loop using hardware-accelerated drawing.
2. **Canvas WASI Backend (`canvas-wasi`)**: Browser-hosted WASI binary driving an HTML5 `<canvas>` via `canvas-wasi` (`graphics_canvas.h`).

---

## 2. Mathematical & Coordinate System
- **Coordinate Space:** Standard Cartesian coordinates.
  - Origin `(0.0, 0.0)` is at the exact center of the viewport/canvas.
  - `+X` points toward the **Right**.
  - `+Y` points toward the **Top** (North).
- **Default Heading:** $90.0^\circ$ (pointing North along $+Y$).
- **Angular Conventions:**
  - Standard math angles: Counter-Clockwise (CCW) rotation is positive.
  - Normalized range: $[0.0, 360.0)$.
- **Floating-Point Precision:** Internal turtle position $(x, y)$, heading, steps, and stroke widths are represented as `double` to prevent truncation and cumulative drifting.

---

## 3. Public API Contract (`turtle.h`)

### 3.1 Colors
Colors are packed 32-bit RGBA integer values (`0xRRGGBBAA`):
- `TURTLE_RGBA(r, g, b, a)`
- Standard 8-color palette macros:
  - `TURTLE_BLACK`   (`0x000000FF`)
  - `TURTLE_RED`     (`0xFF0000FF`)
  - `TURTLE_GREEN`   (`0x00FF00FF`)
  - `TURTLE_BLUE`    (`0x0000FFFF`)
  - `TURTLE_YELLOW`  (`0xFFFF00FF`)
  - `TURTLE_MAGENTA` (`0xFF00FFFF`)
  - `TURTLE_CYAN`    (`0x00FFFFFF`)
  - `TURTLE_WHITE`   (`0xFFFFFFFF`)

### 3.2 Lifecycle
- `bool turtle_begin(void *context);`
  - Initializes backend graphics with a platform-dependent context struct pointer.
  - Queries physical viewport dimensions from HAL.
  - Resets turtle state to home (`(0, 0)`, heading $90^\circ$, pen down, black, stroke 2.0).
- `void turtle_end(void);`
  - Flushes graphics, enters interactive window loop (Raylib), or finalizes canvas state (WASI).
  - Cleans up allocated graphics resources.
- `void turtle_clear(uint32_t color);`
  - Clears entire viewport with the specified RGBA color.
- `void turtle_home(void);`
  - Returns turtle position to `(0, 0)` with heading reset to $90.0^\circ$.
- `void turtle_delay(double ms);`
  - Flushes and presents the canvas so intermediate actions are immediately visible.
  - Pauses execution for `ms` milliseconds to enable progressive drawing and animation while maintaining interactive responsiveness.

### 3.3 Navigation & Steering
- `void turtle_forward(double steps);`
  - Moves forward by `steps` along the current heading vector.
  - Negative values move backwards without changing the heading.
  - If pen is down, draws a line segment using current stroke and color.
- `void turtle_turn(double deg);`
  - Adjusts heading counter-clockwise by `deg`.
- `void turtle_turn_left(double deg);`
  - Alias for `turtle_turn(deg)`.
- `void turtle_turn_right(double deg);`
  - Adjusts heading clockwise (`turtle_turn(-deg)`).

### 3.4 Pen State
- `void turtle_pen_down(void);`
- `void turtle_pen_up(void);`
- `int  turtle_is_pen_down(void);`
- `void turtle_pen_stroke(double stroke);`
- `void turtle_pen_color(uint32_t color);`
- `void turtle_pen(double stroke, uint32_t color, int is_down);`

### 3.5 Cursor Visibility
- `void turtle_set_visible(bool visible);`
  - Shows or hides the stylized turtle cursor indicator (enabled by default).
- `bool turtle_is_visible(void);`
  - Returns whether the turtle cursor indicator is currently visible.

### 3.6 State Inspection
- `double turtle_get_x(void);`
- `double turtle_get_y(void);`
- `double turtle_get_heading(void);`

*Note: Deliberately omitted `turtle_goto()` and `turtle_backward()` to serve as student programming assignments.*

---

## 4. Hardware Abstraction Layer (`turtle_hal.h`)
The core `turtle.c` performs coordinate transformation into physical screen space (origin at top-left, `+Y` down):
- $x_{\text{screen}} = \frac{W}{2} + x_{\text{math}}$
- $y_{\text{screen}} = \frac{H}{2} - y_{\text{math}}$

Every backend must implement the following C signatures:
```c
bool hal_turtle_init(void *context, double *out_width, double *out_height);
void hal_turtle_present(void);
void hal_turtle_delay(double ms);
void hal_turtle_set_cursor(double screen_x, double screen_y, double heading_deg, bool visible);
void hal_turtle_destroy(void);
void hal_turtle_clear(uint32_t bg_color);
void hal_turtle_draw_line(double x1, double y1, double x2, double y2, double stroke_width, uint32_t color);
```

---

## 5. Backend Specifications

### 5.1 Raylib Backend (`backend/raylib/`)

* **Header:** `turtle_backend_raylib.h`
```c
typedef struct {
    int width;
    int height;
    const char *title;
    int target_fps;
} turtle_raylib_context_t;

```


* **Implementation Strategy:**
* Allocate an offscreen `RenderTexture2D` matching window dimensions during `hal_turtle_init`.
* Draw lines directly to the `RenderTexture2D` canvas buffer so previous strokes persist across frames.
* Render the stylized turtle cursor as a non-destructive overlay on the display framebuffer during presentation, keeping the persistent canvas pristine.
* In `hal_turtle_delay()`, delay intervals dynamically respect the configured `target_fps` by timing frames and sleeping sub-frame remainders with `WaitTime()` and `PollInputEvents()`.
* In `hal_turtle_present()`, run `while (!WindowShouldClose())` loop drawing the texture and cursor to the primary framebuffer.

### 5.2 Canvas WASI Backend (`backend/canvas-wasi/`)

* **Header:** `turtle_backend_canvas_wasi.h`
```c
typedef struct {
    int width;
    int height;
    const char *title;
} turtle_canvas_wasi_context_t;
```

* **Target Architecture:** `wasm32-wasi`
* **Bridge Model:**
Integrates with [`canvas-wasi`](https://github.com/breeze833/canvas-wasi) via `<graphics_canvas.h>` and `<graphics.h>`.
- `graphics_init(int width, int height, const char *title)` initializes the canvas display window.
- `canvas_clear_rect(float x, float y, float w, float h)` and `canvas_fill_rect(...)` manage canvas clearing.
- `canvas_begin_path()`, `canvas_move_to(...)`, `canvas_line_to(...)`, `canvas_set_stroke_style(...)`, `canvas_set_line_width(...)`, `canvas_set_line_cap("round")`, and `canvas_stroke()` render vector line segments.
- `canvas_present()` commits/presents the rendered frame to the browser host.
- In `hal_turtle_present()`, run an interactive waiting loop `while (!graphics_should_close() && !graphics_is_key_down(27))` paced with `graphics_wait_frame(60)` until window close or ESC is pressed.
- `graphics_wait_frame(int target_fps)` paces progressive execution in `hal_turtle_delay()` based on requested millisecond delay (`1000.0 / ms`).
- Cursor visualization uses localized pixel snapshots (`canvas_get_image_data`) and restoration (`canvas_put_image_data`) before subsequent line drawing, ensuring the turtle indicator does not corrupt canvas art.
- `graphics_close()` releases graphics resources.
* **Header Synchronization:**
The required headers `graphics.h` and `graphics_canvas.h` are downloaded at build-time from the upstream `canvas-wasi` repository into `backend/canvas-wasi/include/`, with support for a local directory override via `CANVAS_WASI_DIR`.



---

## 6. Build & Packaging Requirements

* Static build-time dispatch selected via `make BACKEND=<type>`:
* `make BACKEND=raylib` $\to$ builds `libturtle_raylib.a`
* `make BACKEND=canvas-wasi` $\to$ builds `libturtle_canvas-wasi.a`


* Single compilation unit output per backend combining `turtle.c` and `backend/<type>/hal_<type>.c`.


