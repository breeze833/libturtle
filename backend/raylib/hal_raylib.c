#include "turtle_hal.h"
#include "turtle_backend_raylib.h"
#include <raylib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#define RGBA_R(c) (((c) >> 24) & 0xFF)
#define RGBA_G(c) (((c) >> 16) & 0xFF)
#define RGBA_B(c) (((c) >> 8)  & 0xFF)
#define RGBA_A(c) ((c)         & 0xFF)

static RenderTexture2D g_target;
static bool g_initialized = false;
static int g_width = 800;
static int g_height = 600;
static int g_target_fps = 60;

static double g_cursor_x = 0.0;
static double g_cursor_y = 0.0;
static double g_cursor_heading = 90.0;
static bool g_cursor_visible = true;

bool hal_turtle_init(void *context, double *out_width, double *out_height) {
    int width = 800;
    int height = 600;
    const char *title = "Turtle Graphics (Raylib)";
    int target_fps = 60;

    if (context != NULL) {
        turtle_raylib_context_t *ctx = (turtle_raylib_context_t *)context;
        if (ctx->width > 0) width = ctx->width;
        if (ctx->height > 0) height = ctx->height;
        if (ctx->title != NULL) title = ctx->title;
        if (ctx->target_fps > 0) target_fps = ctx->target_fps;
    }

    g_width = width;
    g_height = height;
    g_target_fps = target_fps;

    InitWindow(width, height, title);
    SetTargetFPS(target_fps);

    g_target = LoadRenderTexture(width, height);

    /* Initialize canvas background to white */
    BeginTextureMode(g_target);
    ClearBackground(RAYWHITE);
    EndTextureMode();

    *out_width = (double)width;
    *out_height = (double)height;
    g_initialized = true;

    return true;
}

void hal_turtle_clear(uint32_t bg_color) {
    if (!g_initialized) return;

    Color color = (Color){
        (unsigned char)RGBA_R(bg_color),
        (unsigned char)RGBA_G(bg_color),
        (unsigned char)RGBA_B(bg_color),
        (unsigned char)RGBA_A(bg_color)
    };

    BeginTextureMode(g_target);
    ClearBackground(color);
    EndTextureMode();
}

void hal_turtle_draw_line(double x1, double y1, 
                          double x2, double y2, 
                          double stroke_width, 
                          uint32_t color) {
    if (!g_initialized) return;

    Color c = (Color){
        (unsigned char)RGBA_R(color),
        (unsigned char)RGBA_G(color),
        (unsigned char)RGBA_B(color),
        (unsigned char)RGBA_A(color)
    };

    Vector2 start = { (float)x1, (float)y1 };
    Vector2 end   = { (float)x2, (float)y2 };

    BeginTextureMode(g_target);
    DrawLineEx(start, end, (float)stroke_width, c);
    /* Draw caps for smooth connections */
    if (stroke_width > 1.0) {
        DrawCircleV(start, (float)(stroke_width / 2.0), c);
        DrawCircleV(end,   (float)(stroke_width / 2.0), c);
    }
    EndTextureMode();
}

void hal_turtle_set_cursor(double screen_x, double screen_y, double heading_deg, bool visible) {
    g_cursor_x = screen_x;
    g_cursor_y = screen_y;
    g_cursor_heading = heading_deg;
    g_cursor_visible = visible;
}

static inline Vector2 to_cursor_pt(float sx, float sy, float ux, float uy, float vx, float vy, float f, float r) {
    return (Vector2){ sx + f * ux + r * vx, sy + f * uy + r * vy };
}

static void draw_turtle_cursor(float sx, float sy, float heading_deg) {
    float rad = (float)(heading_deg * (PI / 180.0));
    float ux = cosf(rad);
    float uy = -sinf(rad);
    float vx = sinf(rad);
    float vy = cosf(rad);

    Color skin_color = (Color){ 76, 175, 80, 255 };    /* #4CAF50 */
    Color shell_color = (Color){ 46, 125, 50, 255 };   /* #2E7D32 */
    Color outline_color = (Color){ 27, 94, 32, 255 };  /* #1B5E20 */
    Color eye_color = (Color){ 15, 23, 42, 255 };

    /* 1. Tail */
    Vector2 t1 = to_cursor_pt(sx, sy, ux, uy, vx, vy, -7.0f, -2.5f);
    Vector2 t2 = to_cursor_pt(sx, sy, ux, uy, vx, vy, -12.0f, 0.0f);
    Vector2 t3 = to_cursor_pt(sx, sy, ux, uy, vx, vy, -7.0f, 2.5f);
    DrawTriangle(t1, t2, t3, skin_color);
    DrawTriangle(t1, t3, t2, skin_color);
    DrawLineEx(t1, t2, 1.2f, outline_color);
    DrawLineEx(t2, t3, 1.2f, outline_color);

    /* 2. Rear flippers */
    Vector2 rl = to_cursor_pt(sx, sy, ux, uy, vx, vy, -5.5f, -6.5f);
    Vector2 rr = to_cursor_pt(sx, sy, ux, uy, vx, vy, -5.5f, 6.5f);
    DrawCircleV(rl, 2.8f, skin_color);
    DrawCircleLinesV(rl, 2.8f, outline_color);
    DrawCircleV(rr, 2.8f, skin_color);
    DrawCircleLinesV(rr, 2.8f, outline_color);

    /* 3. Front flippers */
    Vector2 fl = to_cursor_pt(sx, sy, ux, uy, vx, vy, 5.5f, -8.0f);
    Vector2 fr = to_cursor_pt(sx, sy, ux, uy, vx, vy, 5.5f, 8.0f);
    DrawCircleV(fl, 3.8f, skin_color);
    DrawCircleLinesV(fl, 3.8f, outline_color);
    DrawCircleV(fr, 3.8f, skin_color);
    DrawCircleLinesV(fr, 3.8f, outline_color);

    /* 4. Head */
    Vector2 head = to_cursor_pt(sx, sy, ux, uy, vx, vy, 11.0f, 0.0f);
    DrawCircleV(head, 4.2f, skin_color);
    DrawCircleLinesV(head, 4.2f, outline_color);

    /* Eyes */
    Vector2 eye_l = to_cursor_pt(sx, sy, ux, uy, vx, vy, 12.0f, -1.8f);
    Vector2 eye_r = to_cursor_pt(sx, sy, ux, uy, vx, vy, 12.0f, 1.8f);
    DrawCircleV(eye_l, 1.0f, eye_color);
    DrawCircleV(eye_r, 1.0f, eye_color);

    /* 5. Carapace (Shell) - Filled oval using center fan */
    #define SHELL_PTS 24
    Vector2 pts[SHELL_PTS];
    Vector2 center = { sx, sy };
    for (int i = 0; i < SHELL_PTS; i++) {
        float a = (float)(i * (2.0 * PI / SHELL_PTS));
        pts[i] = to_cursor_pt(sx, sy, ux, uy, vx, vy, 9.5f * cosf(a), 7.5f * sinf(a));
    }
    for (int i = 0; i < SHELL_PTS; i++) {
        int next = (i + 1) % SHELL_PTS;
        /* Draw counter-clockwise and clockwise triangles to guarantee rendering regardless of culling state */
        DrawTriangle(center, pts[next], pts[i], shell_color);
        DrawTriangle(center, pts[i], pts[next], shell_color);
    }
    for (int i = 0; i < SHELL_PTS; i++) {
        DrawLineEx(pts[i], pts[(i + 1) % SHELL_PTS], 1.5f, outline_color);
    }
    #undef SHELL_PTS

    /* Inner shell pattern (diamond) */
    Vector2 d1 = to_cursor_pt(sx, sy, ux, uy, vx, vy, 4.5f, 0.0f);
    Vector2 d2 = to_cursor_pt(sx, sy, ux, uy, vx, vy, 0.0f, 3.5f);
    Vector2 d3 = to_cursor_pt(sx, sy, ux, uy, vx, vy, -4.5f, 0.0f);
    Vector2 d4 = to_cursor_pt(sx, sy, ux, uy, vx, vy, 0.0f, -3.5f);
    DrawLineEx(d1, d2, 1.2f, outline_color);
    DrawLineEx(d2, d3, 1.2f, outline_color);
    DrawLineEx(d3, d4, 1.2f, outline_color);
    DrawLineEx(d4, d1, 1.2f, outline_color);
}

static void render_screen(void) {
    BeginDrawing();
    ClearBackground(BLACK);

    /* Flip Y because RenderTexture2D in OpenGL is inverted */
    Rectangle src = { 0.0f, 0.0f, (float)g_target.texture.width, -(float)g_target.texture.height };
    Rectangle dst = { 0.0f, 0.0f, (float)g_width, (float)g_height };
    Vector2 origin = { 0.0f, 0.0f };

    DrawTexturePro(g_target.texture, src, dst, origin, 0.0f, WHITE);

    if (g_cursor_visible) {
        draw_turtle_cursor((float)g_cursor_x, (float)g_cursor_y, (float)g_cursor_heading);
    }

    EndDrawing();
}

void hal_turtle_delay(double ms) {
    if (!g_initialized) return;

    if (WindowShouldClose()) {
        hal_turtle_destroy();
        exit(0);
    }

    if (ms <= 0.0) {
        render_screen();
        if (WindowShouldClose()) {
            hal_turtle_destroy();
            exit(0);
        }
        return;
    }

    double frame_interval;
    if (g_target_fps > 0) {
        frame_interval = 1.0 / (double)g_target_fps;
    } else {
        float dt = GetFrameTime();
        frame_interval = (dt > 0.001f) ? (double)dt : (1.0 / 60.0);
    }

    double start = GetTime();
    double duration = ms / 1000.0;

    render_screen();

    while (!WindowShouldClose()) {
        double elapsed = GetTime() - start;
        double remaining = duration - elapsed;
        if (remaining <= 0.001) {
            break;
        }
        /* Only perform another full frame render if at least one frame interval remains */
        if (remaining >= frame_interval) {
            render_screen();
        } else {
            WaitTime(remaining);
            PollInputEvents();
            break;
        }
    }

    if (WindowShouldClose()) {
        hal_turtle_destroy();
        exit(0);
    }
}

void hal_turtle_present(void) {
    if (!g_initialized) return;

    /* Interactive display loop until window close requested */
    while (!WindowShouldClose()) {
        render_screen();
    }
}

void hal_turtle_destroy(void) {
    if (!g_initialized) return;

    UnloadRenderTexture(g_target);
    CloseWindow();
    g_initialized = false;
}
