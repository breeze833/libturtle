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
  - **Canvas WASI Backend (`canvas-wasi`)**: Zero-dependency browser-hosted WebAssembly module driving HTML5 Canvas via [canvas-wasi](https://github.com/breeze833/canvas-wasi) (with headers dynamically fetched at build time), non-destructive cursor snapshot and restore, monotonic clock frame slicing, and interactive loop support.
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
│       │   ├── graphics.h               # Dynamically fetched from canvas-wasi (gitignored)
│       │   └── graphics_canvas.h        # Dynamically fetched from canvas-wasi (gitignored)
│       └── hal_canvas_wasi.c            # canvas-wasi HAL implementation
├── docs/
│   ├── user_guide.md                    # User Guide & Public API Reference
│   └── backend_guide.md                 # Backend Developer Guide & HAL Specification
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

## 📚 Documentation

Detailed documentation is available in the [`docs/`](docs/) directory:
- 📖 **[User Guide & API Reference](docs/user_guide.md)**: A complete guide for students, teachers, and application developers. Covers coordinate systems, high-level turtle commands, backend context setup, progressive animation, and implementation recipes for omitted primitives like `turtle_goto`.
- 🛠️ **[Backend Developer Guide & HAL Specification](docs/backend_guide.md)**: An in-depth guide for engine contributors and graphics developers. Details the decoupled architecture, link-time backend resolution pattern, HAL contract (`turtle_hal.h`), persistence strategies, cursor overlay invariants, and a step-by-step tutorial for implementing new backends.

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
*(Note: Required headers `graphics.h` and `graphics_canvas.h` are automatically fetched from GitHub on build, with optional local directory override via `CANVAS_WASI_DIR`)*
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

## 📖 API Quick Reference

For complete descriptions, parameter types, coordinate math, and tutorials, see the **[User Guide & API Reference](docs/user_guide.md)**. For HAL and backend integration details, see the **[Backend Developer Guide](docs/backend_guide.md)**.

| Category | Functions & Macros | Description |
|---|---|---|
| **Lifecycle** | `turtle_begin(ctx)`, `turtle_end()` | Start backend & state machine; enter interactive loop & cleanup |
| **Canvas** | `turtle_clear(color)`, `turtle_home()`, `turtle_delay(ms)` | Clear background; reset to center; flush & pause for animation |
| **Steering** | `turtle_forward(steps)`, `turtle_turn(deg)`<br>`turtle_turn_left(deg)`, `turtle_turn_right(deg)` | Move along heading (negative = reverse); rotate CCW / CW |
| **Pen** | `turtle_pen_down()`, `turtle_pen_up()`, `turtle_is_pen_down()`<br>`turtle_pen_stroke(w)`, `turtle_pen_color(c)`, `turtle_pen(w, c, down)` | Lower/raise pen; query pen state; configure stroke width and color |
| **Cursor** | `turtle_set_visible(bool)`, `turtle_is_visible()` | Show/hide the on-screen turtle indicator |
| **Inspection** | `turtle_get_x()`, `turtle_get_y()`, `turtle_get_heading()` | Query current Cartesian coordinates and heading angle $[0.0, 360.0)$ |
| **Colors** | `TURTLE_RGBA(r, g, b, a)`, `TURTLE_BLACK`, `TURTLE_WHITE`, ... | 32-bit packed `0xRRGGBBAA` color constructor and standard palette |

> [!NOTE]
> Higher-level navigation routines such as `turtle_goto(x, y)` and `turtle_backward(steps)` are intentionally omitted from the core library as pedagogical assignments. See [Implementing Missing Primitives](docs/user_guide.md#5-pedagogical-notes-missing-primitives) in the User Guide for recipes.

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
make clean                    # Remove build directory and downloaded canvas-wasi headers
```

---

## 📜 License

This project is licensed under the MIT License.
