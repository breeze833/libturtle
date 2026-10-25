#include "turtle_hal.h"
#include "turtle_backend_canvas_wasi.h"
#include "graphics_canvas.h"
#include "graphics.h"
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RGBA_R(c) (((c) >> 24) & 0xFF)
#define RGBA_G(c) (((c) >> 16) & 0xFF)
#define RGBA_B(c) (((c) >> 8)  & 0xFF)
#define RGBA_A(c) ((c)         & 0xFF)

static bool g_initialized = false;
static int g_width = 800;
static int g_height = 600;

static double g_cursor_x = 0.0;
static double g_cursor_y = 0.0;
static double g_cursor_heading = 90.0;
static bool g_cursor_visible = true;

#define CURSOR_BOX_SIZE 48
static uint32_t s_cursor_backup[CURSOR_BOX_SIZE * CURSOR_BOX_SIZE];
static bool s_cursor_on_canvas = false;
static int s_cursor_bx = 0;
static int s_cursor_by = 0;
static int s_cursor_bw = 0;
static int s_cursor_bh = 0;

static void restore_canvas_under_cursor(void);
static void draw_cursor_and_backup(void);

bool hal_turtle_init(void *context, double *out_width, double *out_height) {
    int width = 800;
    int height = 600;
    const char *title = "Turtle Graphics (WASI)";

    if (context != NULL) {
        turtle_canvas_wasi_context_t *ctx = (turtle_canvas_wasi_context_t *)context;
        if (ctx->width > 0) width = ctx->width;
        if (ctx->height > 0) height = ctx->height;
        if (ctx->title != NULL) title = ctx->title;
    }

    g_width = width;
    g_height = height;

    graphics_init(width, height, title);

    /* Initialize background to white */
    canvas_save();
    canvas_set_fill_style("#ffffff");
    canvas_fill_rect(0.0f, 0.0f, (float)width, (float)height);
    canvas_restore();

    *out_width = (double)width;
    *out_height = (double)height;
    g_initialized = true;
    s_cursor_on_canvas = false;

    return true;
}

void hal_turtle_clear(uint32_t bg_color) {
    if (!g_initialized) return;

    s_cursor_on_canvas = false;

    char color_buf[48];
    snprintf(color_buf, sizeof(color_buf), "rgba(%u,%u,%u,%.3f)",
             RGBA_R(bg_color), RGBA_G(bg_color), RGBA_B(bg_color), RGBA_A(bg_color) / 255.0);

    canvas_save();
    canvas_set_fill_style(color_buf);
    canvas_fill_rect(0.0f, 0.0f, (float)g_width, (float)g_height);
    canvas_restore();
}

void hal_turtle_draw_line(double x1, double y1, 
                          double x2, double y2, 
                          double stroke_width, 
                          uint32_t color) {
    if (!g_initialized) return;

    restore_canvas_under_cursor();

    char color_buf[48];
    snprintf(color_buf, sizeof(color_buf), "rgba(%u,%u,%u,%.3f)",
             RGBA_R(color), RGBA_G(color), RGBA_B(color), RGBA_A(color) / 255.0);

    canvas_begin_path();
    canvas_set_stroke_style(color_buf);
    canvas_set_line_width((float)stroke_width);
    canvas_set_line_cap("round");
    canvas_move_to((float)x1, (float)y1);
    canvas_line_to((float)x2, (float)y2);
    canvas_stroke();
}

void hal_turtle_set_cursor(double screen_x, double screen_y, double heading_deg, bool visible) {
    if (s_cursor_on_canvas) {
        restore_canvas_under_cursor();
    }
    g_cursor_x = screen_x;
    g_cursor_y = screen_y;
    g_cursor_heading = heading_deg;
    g_cursor_visible = visible;
}

static void draw_turtle_cursor_canvas(double sx, double sy, double heading_deg) {
    double rad = heading_deg * (M_PI / 180.0);

    canvas_save();
    canvas_translate((float)sx, (float)sy);
    canvas_rotate((float)-rad);

    /* 1. Tail */
    canvas_begin_path();
    canvas_set_fill_style("#4caf50");
    canvas_set_stroke_style("#1b5e20");
    canvas_set_line_width(1.2f);
    canvas_move_to(-7.0f, -2.5f);
    canvas_line_to(-12.0f, 0.0f);
    canvas_line_to(-7.0f, 2.5f);
    canvas_close_path();
    canvas_fill();
    canvas_stroke();

    /* 2. Rear flippers */
    canvas_set_fill_style("#4caf50");
    canvas_set_stroke_style("#1b5e20");
    canvas_set_line_width(1.2f);

    canvas_begin_path();
    canvas_arc(-5.5f, -6.5f, 2.8f, 0.0f, (float)(2.0 * M_PI), false);
    canvas_fill();
    canvas_stroke();

    canvas_begin_path();
    canvas_arc(-5.5f, 6.5f, 2.8f, 0.0f, (float)(2.0 * M_PI), false);
    canvas_fill();
    canvas_stroke();

    /* 3. Front flippers */
    canvas_begin_path();
    canvas_arc(5.5f, -8.0f, 3.8f, 0.0f, (float)(2.0 * M_PI), false);
    canvas_fill();
    canvas_stroke();

    canvas_begin_path();
    canvas_arc(5.5f, 8.0f, 3.8f, 0.0f, (float)(2.0 * M_PI), false);
    canvas_fill();
    canvas_stroke();

    /* 4. Head */
    canvas_begin_path();
    canvas_arc(11.0f, 0.0f, 4.2f, 0.0f, (float)(2.0 * M_PI), false);
    canvas_fill();
    canvas_stroke();

    /* Eyes */
    canvas_set_fill_style("#0f172a");
    canvas_begin_path();
    canvas_arc(12.0f, -1.8f, 1.0f, 0.0f, (float)(2.0 * M_PI), false);
    canvas_fill();
    canvas_begin_path();
    canvas_arc(12.0f, 1.8f, 1.0f, 0.0f, (float)(2.0 * M_PI), false);
    canvas_fill();

    /* 5. Carapace (Shell) */
    canvas_save();
    canvas_scale(1.25f, 1.0f);
    canvas_begin_path();
    canvas_arc(0.0f, 0.0f, 7.5f, 0.0f, (float)(2.0 * M_PI), false);
    canvas_set_fill_style("#2e7d32");
    canvas_set_stroke_style("#1b5e20");
    canvas_set_line_width(1.5f);
    canvas_fill();
    canvas_stroke();
    canvas_restore();

    /* Inner shell pattern (diamond) */
    canvas_set_stroke_style("#1b5e20");
    canvas_set_line_width(1.2f);
    canvas_begin_path();
    canvas_move_to(4.5f, 0.0f);
    canvas_line_to(0.0f, 3.5f);
    canvas_line_to(-4.5f, 0.0f);
    canvas_line_to(0.0f, -3.5f);
    canvas_close_path();
    canvas_stroke();

    canvas_restore();
}

static void restore_canvas_under_cursor(void) {
    if (s_cursor_on_canvas) {
        if (s_cursor_bw > 0 && s_cursor_bh > 0) {
            canvas_put_image_data(s_cursor_bx, s_cursor_by, s_cursor_bw, s_cursor_bh, s_cursor_backup);
        }
        s_cursor_on_canvas = false;
    }
}

static void draw_cursor_and_backup(void) {
    if (s_cursor_on_canvas || !g_cursor_visible) return;

    int bx = (int)g_cursor_x - CURSOR_BOX_SIZE / 2;
    int by = (int)g_cursor_y - CURSOR_BOX_SIZE / 2;
    int bw = CURSOR_BOX_SIZE;
    int bh = CURSOR_BOX_SIZE;
    if (bx < 0) { bw += bx; bx = 0; }
    if (by < 0) { bh += by; by = 0; }
    if (bx + bw > g_width) bw = g_width - bx;
    if (by + bh > g_height) bh = g_height - by;

    s_cursor_bx = bx;
    s_cursor_by = by;
    s_cursor_bw = bw;
    s_cursor_bh = bh;

    if (bw > 0 && bh > 0) {
        canvas_get_image_data(bx, by, bw, bh, s_cursor_backup);
    }

    draw_turtle_cursor_canvas(g_cursor_x, g_cursor_y, g_cursor_heading);
    s_cursor_on_canvas = true;
}

void hal_turtle_present(void) {
    if (!g_initialized) return;

    restore_canvas_under_cursor();
    if (g_cursor_visible) {
        draw_cursor_and_backup();
    }
    canvas_present();
}

void hal_turtle_delay(double ms) {
    if (!g_initialized) return;

    restore_canvas_under_cursor();
    if (g_cursor_visible) {
        draw_cursor_and_backup();
    }
    canvas_present();
    restore_canvas_under_cursor();

    if (ms > 0.0) {
        if (ms <= 1000.0) {
            int target_fps = (int)(1000.0 / ms);
            if (target_fps <= 0) target_fps = 1;
            graphics_wait_frame(target_fps);
        } else {
            int seconds = (int)(ms / 1000.0);
            for (int i = 0; i < seconds; i++) {
                graphics_wait_frame(1);
            }
            double rem = ms - (seconds * 1000.0);
            if (rem > 0.0) {
                int rem_fps = (int)(1000.0 / rem);
                if (rem_fps <= 0) rem_fps = 1;
                graphics_wait_frame(rem_fps);
            }
        }
    }
}

void hal_turtle_destroy(void) {
    if (!g_initialized) return;
    graphics_close();
    g_initialized = false;
}
