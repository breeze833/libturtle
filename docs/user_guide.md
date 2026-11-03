# libturtle User Guide & API Reference

Welcome to `libturtle`, a modern, educational C turtle graphics library designed for computer science pedagogy.

`libturtle` allows you to write clean, platform-agnostic C code to create 2D vector illustrations, geometric fractals, and progressive animations. Your programs can run either as native desktop applications (using Raylib) or directly in a web browser using WebAssembly and HTML5 Canvas (via `canvas-wasi`).

---

## 1. Core Concepts & Coordinate System

### 1.1 The Mathematical Coordinate Plane
Unlike standard computer screen coordinate systems where $(0, 0)$ is at the top-left corner and $+Y$ points downward, `libturtle` uses a **standard Cartesian coordinate system**:

```
                       +Y (North, 90°)
                             │
                             │
       -X (West, 180°) ──────┼────── +X (East, 0°)
                        (0,0)│ Origin
                             │
                             │
                       -Y (South, 270°)
```

- **Origin `(0.0, 0.0)`**: The exact geometric center of the canvas viewport.
- **X Axis**: $+X$ extends to the **Right** (East), $-X$ extends to the **Left** (West).
- **Y Axis**: $+Y$ extends **Upward** (North), $-Y$ extends **Downward** (South).
- **Default Position**: `(0.0, 0.0)`.

### 1.2 Angles and Headings
- **Default Heading**: $90.0^\circ$ (pointing directly North along $+Y$).
- **Angular Direction**: Standard mathematical conventions apply: **counter-clockwise (CCW) rotation is positive**.
  - $0.0^\circ$: East ($+X$)
  - $90.0^\circ$: North ($+Y$)
  - $180.0^\circ$: West ($-X$)
  - $270.0^\circ$: South ($-Y$)
- **Heading Range**: Automatically normalized to $[0.0, 360.0)$.

### 1.3 High-Precision Math
All internal calculations (coordinates, heading angles, step sizes, and stroke widths) use `double` precision floating-point numbers. This avoids cumulative rounding errors and angular drift across complex iterative drawings such as recursive fractals or spirals.

---

## 2. Library Architecture (User Perspective)

`libturtle` uses a decoupled architecture. High-level drawing commands (`turtle_forward`, `turtle_turn`, etc.) are completely independent of the underlying operating system or rendering engine.

To run a turtle program, you only need two components:
1. **The Core Turtle Header**: `#include "turtle.h"`
2. **One Backend Header**: `#include "turtle_backend_raylib.h"` or `#include "turtle_backend_canvas_wasi.h"`

```
┌────────────────────────────────────────────────────────┐
│                   Your C Application                   │
│         (#include "turtle.h" + backend header)         │
└───────────────────────────┬────────────────────────────┘
                            │ Calls turtle_begin(&ctx)
                            ▼
┌────────────────────────────────────────────────────────┐
│             Backend Initializer & Context              │
│       (turtle_raylib_context_t / turtle_canvas_wasi)   │
└───────────────────────────┬────────────────────────────┘
                            │ Initializes
                            ▼
┌────────────────────────────────────────────────────────┐
│                   libturtle Engine                     │
│         (State Machine, Geometry, Math, Pens)          │
└────────────────────────────────────────────────────────┘
```

The user never needs to interact with low-level rendering code or internal Hardware Abstraction Layer (HAL) details. You configure a single context struct for your chosen platform, pass it to `turtle_begin()`, and then use standard turtle commands.

---

## 3. Backend Context Configuration

Because window creation and platform settings vary between desktop operating systems and WebAssembly browsers, each backend provides its own context structure.

### 3.1 Raylib Desktop Backend (`turtle_backend_raylib.h`)
Used for native desktop execution (Linux, macOS, Windows).

```c
#include "turtle_backend_raylib.h"

typedef struct {
    int width;          /* Viewport width in pixels (e.g. 800) */
    int height;         /* Viewport height in pixels (e.g. 600) */
    const char *title;  /* Window title text */
    int target_fps;     /* Target frames per second (e.g. 60) */
} turtle_raylib_context_t;
```

#### Example Initialization:
```c
turtle_raylib_context_t ctx = {
    .width = 800,
    .height = 600,
    .title = "My Desktop Turtle",
    .target_fps = 60
};

if (!turtle_begin(&ctx)) {
    // Window creation failed
    return 1;
}
```

### 3.2 Canvas WASI Backend (`turtle_backend_canvas_wasi.h`)
Used for compiling to WebAssembly (`wasm32-wasi`) to render directly onto an HTML5 `<canvas>` inside web browsers.

```c
#include "turtle_backend_canvas_wasi.h"

typedef struct {
    int width;          /* Canvas width in pixels (e.g. 800) */
    int height;         /* Canvas height in pixels (e.g. 600) */
    const char *title;  /* Canvas window title */
} turtle_canvas_wasi_context_t;
```

#### Example Initialization:
```c
turtle_canvas_wasi_context_t ctx = {
    .width = 800,
    .height = 600,
    .title = "My Web Turtle"
};

if (!turtle_begin(&ctx)) {
    // Canvas initialization failed
    return 1;
}
```

> [!TIP]
> Notice the **Link-Time Resolution Safety**: If you accidentally include `turtle_backend_raylib.h` in your code but link against `libturtle_canvas-wasi.a`, the compiler linker will immediately fail with an unresolved symbol error (`undefined reference to raylib_turtle_init`). This design prevents passing incompatible context configurations at runtime.

---

## 4. Complete Public API Reference (`turtle.h`)

### 4.1 Color Definition & Palettes

Colors in `libturtle` are packed 32-bit unsigned integers in `0xRRGGBBAA` format.

#### Custom Color Macro
```c
TURTLE_RGBA(r, g, b, a)
```
Constructs a packed 32-bit RGBA color from four 8-bit components (0 to 255).
- `r`: Red component ($0 - 255$)
- `g`: Green component ($0 - 255$)
- `b`: Blue component ($0 - 255$)
- `a`: Alpha (opacity) component ($0$ is transparent, $255$ is completely opaque)

*Example:*
```c
uint32_t orange = TURTLE_RGBA(255, 165, 0, 255);
uint32_t semi_transparent_blue = TURTLE_RGBA(0, 0, 255, 128);
```

#### Standard 8-Color Palette Constants
| Macro | Hex Value | Color Preview |
|---|---|---|
| `TURTLE_BLACK` | `0x000000FF` | Solid Black |
| `TURTLE_RED` | `0xFF0000FF` | Solid Red |
| `TURTLE_GREEN` | `0x00FF00FF` | Solid Green |
| `TURTLE_BLUE` | `0x0000FFFF` | Solid Blue |
| `TURTLE_YELLOW` | `0xFFFF00FF` | Solid Yellow |
| `TURTLE_MAGENTA` | `0xFF00FFFF` | Solid Magenta |
| `TURTLE_CYAN` | `0x00FFFFFF` | Solid Cyan |
| `TURTLE_WHITE` | `0xFFFFFFFF` | Solid White |

---

### 4.2 Lifecycle & Canvas Control

#### `turtle_begin(context)`
```c
bool turtle_begin(void *context);
```
- **Description**: Convenience macro provided by the backend header. It initializes the backend graphics system using the supplied context and configures the turtle state machine.
- **Parameters**: Pointer to backend context struct (e.g. `&ctx`).
- **Returns**: `true` on success, `false` on initialization failure.
- **Initial State**:
  - Position: `(0.0, 0.0)`
  - Heading: $90.0^\circ$ (North)
  - Pen: Down, Black (`TURTLE_BLACK`), Stroke Width $2.0\text{ px}$
  - Cursor: Visible

#### `void turtle_end(void)`
- **Description**: Concludes drawing operations. Enters an interactive event loop keeping the window or browser canvas responsive until the user closes the window or presses `ESC`. Upon exit, releases all graphics memory.
- **Usage**: Always call `turtle_end()` at the end of `main()`.

#### `void turtle_clear(uint32_t color)`
- **Description**: Clears the entire canvas viewport and fills it with the specified RGBA background color.
- **Parameters**:
  - `color`: Packed RGBA color (e.g. `TURTLE_WHITE`).

#### `void turtle_home(void)`
- **Description**: Resets the turtle's position to the center of the canvas `(0.0, 0.0)` and resets the heading to $90.0^\circ$ (North). Does not alter pen settings or clear existing drawings.

#### `void turtle_delay(double ms)`
- **Description**: Immediately flushes all pending drawing commands to the screen and pauses execution for `ms` milliseconds.
- **Parameters**:
  - `ms`: Duration to pause in milliseconds.
- **Why use this?**: By placing `turtle_delay()` inside iterative loops, you create real-time progressive drawing animations. The application remains interactive and will exit cleanly if the user closes the window or presses `ESC` during the delay.

---

### 4.3 Movement & Steering

#### `void turtle_forward(double steps)`
- **Description**: Moves the turtle forward along its current heading by `steps` pixels.
  - If the pen is down, a line segment is drawn between the old and new coordinates using the current pen stroke and color.
  - If the pen is up, the turtle relocates without drawing.
  - If `steps` is negative, the turtle moves in reverse without modifying its heading angle.
- **Parameters**:
  - `steps`: Distance to move in pixels (can be positive or negative).

#### `void turtle_turn(double deg)`
- **Description**: Rotates the turtle counter-clockwise by `deg` degrees.
- **Parameters**:
  - `deg`: Angle to rotate in degrees (positive = counter-clockwise, negative = clockwise).

#### `void turtle_turn_left(double deg)`
- **Description**: Rotates the turtle counter-clockwise (to the left) by `deg` degrees. Equivalent to `turtle_turn(deg)`.

#### `void turtle_turn_right(double deg)`
- **Description**: Rotates the turtle clockwise (to the right) by `deg` degrees. Equivalent to `turtle_turn(-deg)`.

---

### 4.4 Pen State Management

#### `void turtle_pen_down(void)`
- **Description**: Lowers the pen. Subsequent movements using `turtle_forward()` will draw lines.

#### `void turtle_pen_up(void)`
- **Description**: Lifts the pen up. The turtle can move to new positions without drawing lines.

#### `int turtle_is_pen_down(void)`
- **Description**: Checks whether the pen is currently lowered.
- **Returns**: `1` if the pen is down, `0` if the pen is up.

#### `void turtle_pen_stroke(double stroke)`
- **Description**: Sets the width of drawn lines in pixels.
- **Parameters**:
  - `stroke`: Line width in pixels (e.g. `2.5`).

#### `void turtle_pen_color(uint32_t color)`
- **Description**: Sets the drawing color for subsequent strokes.
- **Parameters**:
  - `color`: Packed 32-bit RGBA color.

#### `void turtle_pen(double stroke, uint32_t color, int is_down)`
- **Description**: Utility function to configure all pen attributes in a single call.
- **Parameters**:
  - `stroke`: Line width in pixels.
  - `color`: Packed RGBA color.
  - `is_down`: `1` to lower pen, `0` to raise pen.

---

### 4.5 Cursor Indicator Visibility

#### `void turtle_set_visible(bool visible)`
- **Description**: Shows or hides the stylized on-screen turtle cursor (which indicates current position and heading).
- **Parameters**:
  - `visible`: `true` to display the cursor, `false` to hide it.

#### `bool turtle_is_visible(void)`
- **Description**: Returns the current visibility state of the turtle cursor.
- **Returns**: `true` if visible, `false` if hidden.

---

### 4.6 State Inspection

These functions query the internal state of the turtle. They are particularly valuable for building custom geometric algorithms, telemetry displays, or implementing higher-level navigation routines.

#### `double turtle_get_x(void)`
- **Description**: Returns the current Cartesian X coordinate (relative to center $(0,0)$).

#### `double turtle_get_y(void)`
- **Description**: Returns the current Cartesian Y coordinate (relative to center $(0,0)$).

#### `double turtle_get_heading(void)`
- **Description**: Returns the current heading angle in degrees within the normalized range $[0.0, 360.0)$.

---

## 5. Pedagogical Notes: Missing Primitives

If you have used turtle graphics in Python or Logo, you may notice that `turtle_goto()` and `turtle_backward()` are **deliberately omitted** from `libturtle`.

> [!NOTE]
> In accordance with educational goals for undergraduate programming pedagogy, `turtle_goto(x, y)` and `turtle_backward(steps)` are intentionally not provided in the library. Implementing these functions serves as an ideal hands-on assignment on trigonometry, vector mathematics, and angle normalization.

### 5.1 Implementing `my_backward()`
Moving backward can be implemented simply by moving forward by a negative distance:

```c
void my_backward(double steps) {
    turtle_forward(-steps);
}
```

### 5.2 Implementing `my_goto()`
To move from the current position $(x_1, y_1)$ to a target position $(x_2, y_2)$ without teleporting, you can calculate the distance and bearing using `<math.h>`:

```c
#include "turtle.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void my_goto(double target_x, double target_y) {
    double cur_x = turtle_get_x();
    double cur_y = turtle_get_y();
    
    double dx = target_x - cur_x;
    double dy = target_y - cur_y;
    double distance = sqrt(dx * dx + dy * dy);
    
    if (distance < 1e-6) {
        return; // Already at the target position
    }
    
    /* Calculate target angle in standard math coordinates (0 = East, 90 = North) */
    double target_angle_rad = atan2(dy, dx);
    double target_angle_deg = target_angle_rad * (180.0 / M_PI);
    if (target_angle_deg < 0.0) {
        target_angle_deg += 360.0;
    }
    
    /* Rotate toward target angle */
    double cur_heading = turtle_get_heading();
    double turn_angle = target_angle_deg - cur_heading;
    
    turtle_turn(turn_angle);
    turtle_forward(distance);
}
```

---

## 6. Practical Tutorials & Code Examples

### 6.1 Unified Cross-Platform Template
You can write code that automatically compiles for both desktop (Raylib) and browser (WASI) using conditional compilation `#if defined(__wasm__)`:

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
        .title = "WASI Turtle Window"
    };
#else
    turtle_raylib_context_t ctx = {
        .width = 800,
        .height = 600,
        .title = "Desktop Turtle Window",
        .target_fps = 60
    };
#endif

    if (!turtle_begin(&ctx)) {
        return 1;
    }

    turtle_clear(TURTLE_WHITE);

    /* --- Drawing Code Goes Here --- */

    turtle_end();
    return 0;
}
```

---

### 6.2 Example: Animated Rainbow Spiral
This example demonstrates progressive drawing, changing pen colors dynamically, and adjusting line widths:

```c
#include "turtle.h"
#if defined(__wasm__)
#include "turtle_backend_canvas_wasi.h"
#else
#include "turtle_backend_raylib.h"
#endif

static const uint32_t PALETTE[6] = {
    TURTLE_RED, TURTLE_YELLOW, TURTLE_GREEN,
    TURTLE_CYAN, TURTLE_BLUE, TURTLE_MAGENTA
};

int main(void) {
#if defined(__wasm__)
    turtle_canvas_wasi_context_t ctx = { 800, 600, "Rainbow Spiral" };
#else
    turtle_raylib_context_t ctx = { 800, 600, "Rainbow Spiral", 60 };
#endif

    if (!turtle_begin(&ctx)) return 1;

    turtle_clear(TURTLE_BLACK);
    turtle_pen_stroke(2.0);

    /* Draw an expanding hexagonal spiral */
    double step_size = 2.0;
    for (int i = 0; i < 90; i++) {
        turtle_pen_color(PALETTE[i % 6]);
        turtle_forward(step_size);
        turtle_turn_left(59.0); // Slightly less than 60° creates rotation
        step_size += 2.5;

        turtle_delay(20.0); // 20ms pause per step for animation
    }

    turtle_end();
    return 0;
}
```

---

### 6.3 Example: Teleporting Without Drawing (Pen Up/Down)
This example draws multiple disconnected shapes:

```c
void draw_box(double size, uint32_t color) {
    turtle_pen_color(color);
    turtle_pen_down();
    for (int i = 0; i < 4; i++) {
        turtle_forward(size);
        turtle_turn_left(90.0);
    }
    turtle_pen_up();
}

int main(void) {
    /* ... initialize context and turtle_begin(&ctx) ... */

    turtle_clear(TURTLE_WHITE);
    turtle_pen_stroke(3.0);
    turtle_pen_up();

    /* Draw left box */
    turtle_forward(-200.0);
    draw_box(100.0, TURTLE_RED);

    /* Return to center and draw right box */
    turtle_home();
    turtle_turn_right(90.0); // Turn East
    turtle_forward(100.0);
    turtle_turn_left(90.0);  // Reorient North
    draw_box(100.0, TURTLE_BLUE);

    turtle_end();
    return 0;
}
```

---

## 7. Compiling and Linking Your Application

### 7.1 Desktop (Raylib) Compilation
Ensure you have `libraylib` installed on your development machine.

Compile and link with `libturtle_raylib.a`:
```bash
gcc -Wall -O2 \
    -Ipath/to/libturtle/include \
    -Ipath/to/libturtle/backend/raylib/include \
    my_program.c \
    path/to/libturtle/build/raylib/libturtle_raylib.a \
    -lraylib -lm -lpthread -ldl \
    -o my_turtle_app

./my_turtle_app
```

### 7.2 WebAssembly (WASI) Compilation
Ensure you have `clang` targeting `wasm32-wasi` and WASI sysroot:

```bash
clang --target=wasm32-wasi -O2 \
    -Ipath/to/libturtle/include \
    -Ipath/to/libturtle/backend/canvas-wasi/include \
    my_program.c \
    path/to/libturtle/build/canvas-wasi/libturtle_canvas-wasi.a \
    -mexec-model=reactor \
    -o my_turtle_app.wasm
```

You can then serve the WASM file using the web runner in `examples/index.html`.
