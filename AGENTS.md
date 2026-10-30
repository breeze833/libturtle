# Instructions for Autonomous AI Agents

You are tasked with maintaining, implementing, and validating the C Turtle Graphics Library as defined in `SPEC.md`. Adhere strictly to the project structure and development standards outlined below.

---

## 1. Directory Structure Rules

```

.
├── include/
│   ├── turtle.h                       # Common public API
│   └── turtle_hal.h                   # Internal HAL contract
├── src/
│   └── turtle.c                       # Core engine & state machine
├── backend/
│   ├── raylib/
│   │   ├── include/
│   │   │   └── turtle_backend_raylib.h # Raylib context definition
│   │   └── hal_raylib.c               # Raylib HAL implementation
│   └── canvas-wasi/
│       ├── include/
│       │   ├── turtle_backend_canvas_wasi.h # WASI Canvas context definition
│       │   ├── graphics.h                  # Synced from canvas-wasi
│       │   └── graphics_canvas.h           # Synced from canvas-wasi
│       └── hal_canvas_wasi.c          # canvas-wasi HAL implementation
├── examples/
│   ├── raylib_demo.c                  # Native desktop example
│   ├── wasm_demo.c                    # WASI web canvas example
│   ├── index.html                     # Browser runner UI
│   └── worker.js                      # Web Worker WASI runner
├── Makefile                           # Unified build system
├── SPEC.md
└── AGENTS.md

```

---

## 2. Coding & Implementation Guidelines

1. **State Machine Integrity:**
   - All turtle state (`x`, `y`, `heading_deg`, `stroke`, `color`, `is_down`, `is_visible`) lives inside `src/turtle.c`.
   - Never expose backend-specific structures or types inside `turtle.h` or `turtle_hal.h`.
   - Keep line drawing strictly decoupled from turtle orientation: `hal_turtle_draw_line()` only receives screen coordinates `(x1, y1)` and `(x2, y2)`. Orientation is passed exclusively to `hal_turtle_set_cursor()` for cursor visualization.

2. **Pedagogical Constraints:**
   - **Do NOT implement `turtle_goto()` or `turtle_backward()`** in the core library under any circumstances. These are reserved assignment problems for students.
   - Any helper calculations (e.g. angle normalization) must use standard math functions (`cos`, `sin`, `fmod`, `atan2`) from `<math.h>`.

3. **Color Formatting:**
   - Always treat colors as packed `uint32_t` `0xRRGGBBAA`.
   - Provide clean extraction macros inside HAL implementations:
     ```c
     #define RGBA_R(c) (((c) >> 24) & 0xFF)
     #define RGBA_G(c) (((c) >> 16) & 0xFF)
     #define RGBA_B(c) (((c) >> 8)  & 0xFF)
     #define RGBA_A(c) ((c)         & 0xFF)
     ```

4. **Raylib Persistence & Timing:**
   - In `backend/raylib/hal_raylib.c`, avoid redrawing line primitives every frame inside the main loop. Draw permanently onto a `RenderTexture2D` during `hal_turtle_draw_line()`, and render that texture during `hal_turtle_present()`.
   - Render the stylized turtle cursor as a non-destructive overlay on the display framebuffer during presentation.
   - In `hal_turtle_delay()`, dynamically respect the configured `target_fps` by timing frames and sleeping sub-frame remainders with `WaitTime()` and `PollInputEvents()`. Do not assume a fixed 60 FPS. Clean up resources and exit promptly if window close is requested during the delay.

5. **canvas-wasi Integration:**
   - In `backend/canvas-wasi/hal_canvas_wasi.c`, implement the HAL using `<graphics_canvas.h>` and `<graphics.h>`.
   - The required headers are synced at build-time from `breeze833/canvas-wasi` via the Makefile.
   - In `hal_turtle_delay()`, pace frames using a monotonic clock loop sliced into short frame intervals (up to 60 FPS / ~16ms) with `graphics_wait_frame()`, polling `graphics_should_close()` for prompt termination. Clean up resources and exit promptly if window close is requested.
   - In `hal_turtle_present()`, run an interactive waiting loop `while (!graphics_should_close() && !graphics_is_key_down(27))` paced with `graphics_wait_frame(60)` until window close or ESC is pressed.
   - Manage cursor visibility by non-destructively snapshotting (`canvas_get_image_data`) and restoring (`canvas_put_image_data`) the canvas area under the cursor before subsequent line draws.

6. **Backend Link-Time Resolution & Initialization:**
   - To prevent linking against an incorrect backend library when consumer code passes contexts as `void*`, each backend provides a uniquely named initializer (`raylib_turtle_init` in `turtle_backend_raylib.h` / `hal_raylib.c`, and `canvas_wasi_turtle_init` in `turtle_backend_canvas_wasi.h` / `hal_canvas_wasi.c`).
   - Backend headers provide the convenience macro `#define turtle_begin(ctx) (<backend>_turtle_init(ctx) && turtle_begin_core())`.
   - `turtle_begin_core(void)` lives in `src/turtle.c` and initializes platform-agnostic state machine variables, querying canvas dimensions via `hal_turtle_get_canvas_size()`.
   - Do not bypass this mechanism by exposing a generic `turtle_begin()` symbol in `src/turtle.c`.

---

## 3. Build & Test Commands

### 3.1 Raylib Backend
- **Build library:**
  ```bash
  make BACKEND=raylib
  ```
- **Build and run demo:**
  ```bash
  make run BACKEND=raylib
  # or ./build/raylib/demo
  ```

### 3.2 Canvas WASI Backend
- **Compiler:** `clang --target=wasm32-wasi`
- **Build library:**
  ```bash
  make BACKEND=canvas-wasi
  ```
- **Build and serve demo:**
  ```bash
  make run BACKEND=canvas-wasi
  # or make serve, then navigate to http://localhost:8000/examples/
  ```

---

## 4. Verification Checklist Before Committing Changes

* [ ] Does `turtle.h` contain only platform-agnostic signatures?
* [ ] Is internal math calculation using `double` and strictly respecting Cartesian $+Y$ North conventions?
* [ ] Does the Makefile produce `libturtle_<backend>.a` cleanly under `build/<backend>/`?
* [ ] Are `turtle_goto` and `turtle_backward` absent from `turtle.h` and `turtle.c`?
* [ ] Does `turtle_begin` expand to backend-specific initializer and `turtle_begin_core` to enforce link-time resolution?



