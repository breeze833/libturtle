# libturtle

A cross-platform, modular educational C turtle graphics library designed for computer science pedagogy.

`libturtle` decouples high-level geometric navigation logic from rasterization backends using a build-time Hardware Abstraction Layer (HAL). It allows students and developers to write standard C turtle graphics programs that run either natively on desktop computers or inside web browsers via WebAssembly.

---

## ✨ Features

- 📐 **Standard Cartesian Coordinate System**:
  - Origin `(0.0, 0.0)` is centered in the viewport.
  - $+X$ points East (Right), $+Y$ points North (Up).
  - Default heading is $90.0^\circ$ (pointing North).
  - Standard mathematical counter-clockwise (CCW) rotation angles.
- 🎯 **Double-Precision Arithmetic**: Position, angles, step sizes, and strokes use `double` to prevent accumulation drift.
- ⏱️ **Progressive Drawing**: Support for step-by-step animation via `turtle_delay(ms)` with sub-frame pacing so students can watch algorithms draw in real time.
- 🐢 **Stylized Turtle Indicator**: Visual position and direction cursor with carapace, flippers, head, and eyes enabled by default, toggled with `turtle_set_visible(bool)`.
- 🎨 **Packed 32-bit RGBA Colors**: Palette macros (`TURTLE_RED`, `TURTLE_BLUE`, etc.) and customizable `TURTLE_RGBA(r, g, b, a)`.
- 🔌 **Pluggable Backends**:
  - **Raylib Desktop Backend (`raylib`)**: Native desktop window with configurable target FPS, dynamic pacing, prompt exit on window close during delay, and hardware-accelerated persistence via `RenderTexture2D`.
  - **Canvas WASI Backend (`canvas-wasi`)**: Zero-dependency browser-hosted WebAssembly module driving HTML5 Canvas via [canvas-wasi](https://github.com/breeze833/canvas-wasi) with non-destructive cursor snapshot and restore, monotonic clock frame slicing, and interactive loop support.
- 🎓 **Pedagogical Design**: Core primitives are minimal and robust; higher-level routines like `turtle_goto()` and `turtle_backward()` are deliberately reserved as student programming assignments.

---

## 📁 Repository Structure

```
.
├── include/
│   ├── turtle.h                         # Public C API
│   └── turtle_hal.h                     # Hardware Abstraction Layer contract
├── src/
│   └── turtle.c                         # Core state machine & geometry math
├── backend/
│   ├── raylib/                          # Desktop native backend
│   │   ├── include/
│   │   │   └── turtle_backend_raylib.h  # Raylib context definition
│   │   └── hal_raylib.c                 # Raylib HAL implementation
│   └── canvas-wasi/                     # WebAssembly / WASI backend
│       ├── include/
│       │   ├── turtle_backend_canvas_wasi.h # Context definition
│       │   ├── graphics.h               # Synced from canvas-wasi
│       │   └── graphics_canvas.h        # Synced from canvas-wasi
│       └── hal_canvas_wasi.c            # canvas-wasi HAL implementation
├── examples/
│   ├── raylib_demo.c                    # Native desktop demo
│   ├── wasm_demo.c                      # WebAssembly WASI demo
│   ├── index.html                       # Standalone browser runner UI
│   └── worker.js                        # Web Worker WASI runner
├── Makefile                             # Unified build system
├── SPEC.md                              # Technical specification
├── AGENTS.md                            # Autonomous agent development instructions
└── README.md
```

---

## 🚀 Quick Start

### 1. Raylib Desktop Backend

#### Prerequisites
- A C compiler (`gcc` or `clang`)
- [Raylib](https://www.raylib.com/) library (`libraylib`) installed on your system.

#### Build & Run
```bash
# Build the static library (build/raylib/libturtle_raylib.a)
make BACKEND=raylib

# Build and run the desktop demo
make run BACKEND=raylib
# Or manually run:
# ./build/raylib/demo
```

---

### 2. Canvas WASI Backend (WebAssembly)

#### Prerequisites
- `clang` with WebAssembly support (`--target=wasm32-wasi`)
- `wasi-libc` (or WASI SDK)
- `python3` (to host the local demo web server)

#### Build & Run in Browser
```bash
# Build the WASM module and launch the local web server
make run BACKEND=canvas-wasi
```

Open your browser to:
👉 **[http://localhost:8000/examples/](http://localhost:8000/examples/)**

You will see the turtle progressively drawing in real-time onto an HTML5 `<canvas>` inside a Web Worker, with interactive controls to restart or exit (ESC).

---

## 💻 Code Example

Here is an example program drawing a square with rounded progressive steps:

```c
#include "turtle.h"

#if defined(__wasm__)
#include "turtle_backend_canvas_wasi.h"
#else
#include "turtle_backend_raylib.h"
#endif

int main(void) {
#if defined(__wasm__)
    turtle_canvas_wasi_context_t ctx = {
        .width = 800,
        .height = 600,
        .title = "WASI Turtle"
    };
#else
    turtle_raylib_context_t ctx = {
        .width = 800,
        .height = 600,
        .title = "Desktop Turtle",
        .target_fps = 60
    };
#endif

    if (!turtle_begin(&ctx)) {
        return 1;
    }

    turtle_clear(TURTLE_WHITE);
    turtle_pen_stroke(3.0);
    turtle_pen_color(TURTLE_BLUE);

    for (int i = 0; i < 4; i++) {
        turtle_forward(120.0);
        turtle_delay(150.0);       // Pause 150ms to animate the step
        turtle_turn_left(90.0);
        turtle_delay(100.0);
    }

    turtle_end(); // Keeps desktop window or browser canvas open in interactive event loop
    return 0;
}
```

---

## 📖 API Reference (`turtle.h`)

### Lifecycle & Viewport
| Function | Description |
|---|---|
| `bool turtle_begin(void *context)` | Initializes graphics backend and resets turtle to home `(0, 0)`. |
| `void turtle_end(void)` | Enters interactive event loop (Raylib & WASI) until closed or ESC pressed, then cleans up resources. |
| `void turtle_clear(uint32_t color)` | Clears viewport with packed RGBA color. |
| `void turtle_home(void)` | Returns turtle to `(0, 0)` with heading reset to $90.0^\circ$ (North). |
| `void turtle_delay(double ms)` | Flushes canvas and pauses for `ms` milliseconds for progressive drawing while handling window close / exit promptly. |

### Steering & Movement
| Function | Description |
|---|---|
| `void turtle_forward(double steps)` | Moves along current heading (negative values move backward without rotating). |
| `void turtle_turn(double deg)` | Rotates heading counter-clockwise by `deg` degrees. |
| `void turtle_turn_left(double deg)` | Counter-clockwise turn alias (`turtle_turn(deg)`). |
| `void turtle_turn_right(double deg)` | Clockwise turn (`turtle_turn(-deg)`). |

### Pen Management
| Function | Description |
|---|---|
| `void turtle_pen_down(void)` | Lowers the pen (drawing enabled). |
| `void turtle_pen_up(void)` | Lifts the pen (movement without drawing). |
| `int turtle_is_pen_down(void)` | Returns `1` if pen is down, `0` otherwise. |
| `void turtle_pen_stroke(double stroke)` | Sets stroke width in pixels. |
| `void turtle_pen_color(uint32_t color)` | Sets stroke color in `0xRRGGBBAA` format. |
| `void turtle_pen(double stroke, uint32_t color, int is_down)` | Configures all pen attributes at once. |

### Cursor Visibility
| Function | Description |
|---|---|
| `void turtle_set_visible(bool visible)` | Shows (`true`) or hides (`false`) the stylized turtle cursor (default: shown). |
| `bool turtle_is_visible(void)` | Returns current visibility state of the turtle cursor. |

### State Inspection
| Function | Description |
|---|---|
| `double turtle_get_x(void)` | Current math $X$ coordinate. |
| `double turtle_get_y(void)` | Current math $Y$ coordinate. |
| `double turtle_get_heading(void)` | Current heading angle in degrees $[0.0, 360.0)$. |

---

## 🛠️ Build System Targets

The unified `Makefile` supports several targets:

```bash
make BACKEND=raylib           # Build libturtle_raylib.a
make demo BACKEND=raylib      # Compile Raylib demo executable
make run BACKEND=raylib       # Build and launch desktop demo

make BACKEND=canvas-wasi      # Build libturtle_canvas-wasi.a for WASI
make demo BACKEND=canvas-wasi # Compile WASM demo module (demo.wasm)
make run BACKEND=canvas-wasi  # Build demo and serve locally on port 8000
make serve                    # Start local web server on port 8000

make fetch-canvas-headers     # Synchronize latest canvas-wasi headers from GitHub
make clean                    # Remove build artifacts
```

---

## 📜 License

This project is licensed under the MIT License.
