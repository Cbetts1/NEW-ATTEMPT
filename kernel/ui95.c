/* =============================================================================
 * AI Aura OS — Windows 95–Themed VGA Text UI Implementation
 *
 * All drawing goes directly to the VGA text buffer at 0xB8000.
 * Screen: 80 columns × 25 rows.  Row 24 is the taskbar.
 * =============================================================================*/

#include "ui95.h"
#include "keyboard.h"
#include "vga.h"
#include <stddef.h>
#include <stdint.h>

#define VGA_COLS 80
#define VGA_ROWS 25

/* The VGA text buffer — each cell is (attr << 8) | char */
static volatile uint16_t * const VRAM = (volatile uint16_t *)0xB8000;

/* ============================================================================
 * Low-level primitives
 * ============================================================================*/

void ui95_put_at(int x, int y, char c, uint8_t attr) {
    if ((unsigned)x >= VGA_COLS || (unsigned)y >= VGA_ROWS) return;
    VRAM[y * VGA_COLS + x] = (uint16_t)((uint8_t)c | ((uint16_t)attr << 8));
}

void ui95_print_at(int x, int y, const char *s, uint8_t attr) {
    while (*s && x < VGA_COLS) {
        ui95_put_at(x++, y, *s++, attr);
    }
}

void ui95_print_dec_at(int x, int y, uint32_t val, uint8_t attr) {
    char buf[12];
    int  i = 0;
    if (val == 0) { ui95_put_at(x, y, '0', attr); return; }
    while (val > 0) { buf[i++] = (char)('0' + val % 10); val /= 10; }
    int j;
    for (j = 0; j < i; j++)
        ui95_put_at(x + j, y, buf[i - 1 - j], attr);
}

void ui95_fill_rect(int x, int y, int w, int h, char c, uint8_t attr) {
    int row, col;
    for (row = y; row < y + h && row < VGA_ROWS; row++)
        for (col = x; col < x + w && col < VGA_COLS; col++)
            ui95_put_at(col, row, c, attr);
}

/* ============================================================================
 * Widgets
 * ============================================================================*/

/* Measure null-terminated string length (no libc) */
static int slen(const char *s) {
    int n = 0; while (s[n]) n++; return n;
}

/* Copy at most n-1 chars */
static void scopy(char *dst, const char *src, int n) {
    int i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void ui95_draw_taskbar(const char *open_app) {
    int row = VGA_ROWS - 1;
    /* Full taskbar strip */
    ui95_fill_rect(0, row, VGA_COLS, 1, ' ', W95_TASKBAR);
    /* [Start] button — blue, bold, at far left */
    ui95_print_at(0, row, " \x10 Start ", W95_START);   /* ► Start */
    /* Vertical separator after Start */
    ui95_put_at(8, row, '\xB3', W95_SHADOW);
    /* Open application label */
    if (open_app && open_app[0]) {
        ui95_print_at(10, row, open_app, W95_SELECTED);
    }
    /* Clock placeholder at far right */
    ui95_print_at(72, row, " 00:00  ", W95_TASKBAR);
}

void ui95_clear_desktop(void) {
    ui95_fill_rect(0, 0, VGA_COLS, VGA_ROWS - 1, ' ', W95_DESKTOP);
}

void ui95_draw_window(int x, int y, int w, int h,
                      const char *title, uint8_t active) {
    if (w < 4 || h < 2) return;
    uint8_t tbattr = active ? W95_TITLEBAR : W95_INACTIVE;

    /* ── Title bar (row y) ──────────────────────────────────────────────── */
    ui95_fill_rect(x, y, w, 1, ' ', tbattr);

    /* Title text — leave 7 chars on right for [_][X] */
    char tbuf[80];
    int maxt = w - 8;
    if (maxt < 1) maxt = 1;
    scopy(tbuf, title, maxt > 79 ? 79 : maxt + 1);
    ui95_print_at(x + 1, y, tbuf, tbattr);

    /* Control buttons */
    if (w >= 8) {
        ui95_print_at(x + w - 7, y, " [_][X]", tbattr);
    }

    /* ── Client area with side borders (rows y+1 … y+h-2) ─────────────── */
    int row;
    for (row = y + 1; row < y + h - 1 && row < VGA_ROWS; row++) {
        ui95_put_at(x,         row, BOX_V,  W95_WINDOW);
        ui95_fill_rect(x + 1, row, w - 2, 1, ' ', W95_WINDOW);
        ui95_put_at(x + w - 1, row, BOX_V,  W95_WINDOW);
    }

    /* ── Bottom border (row y+h-1) ─────────────────────────────────────── */
    int br = y + h - 1;
    if (br < VGA_ROWS) {
        int col;
        ui95_put_at(x, br, BOX_BL, W95_WINDOW);
        for (col = x + 1; col < x + w - 1; col++)
            ui95_put_at(col, br, BOX_H, W95_WINDOW);
        ui95_put_at(x + w - 1, br, BOX_BR, W95_WINDOW);
    }
}

/* ============================================================================
 * Startup Splash
 * ============================================================================*/

void ui95_startup_splash(void) {
    int c, r, i;

    /* ── Blue boot screen ───────────────────────────────────────────────── */
    ui95_fill_rect(0, 0, VGA_COLS, VGA_ROWS, ' ', W95_SPLASH);

    /* ── Outer box (double-line border) ────────────────────────────────── */
    int bx = 18, by = 5, bw = 44, bh = 15;

    /* Corners + top/bottom edges */
    ui95_put_at(bx,         by,         '\xC9', 0x1F);   /* ╔ */
    ui95_put_at(bx + bw-1,  by,         '\xBB', 0x1F);   /* ╗ */
    ui95_put_at(bx,         by + bh-1,  '\xC8', 0x1F);   /* ╚ */
    ui95_put_at(bx + bw-1,  by + bh-1,  '\xBC', 0x1F);   /* ╝ */
    for (c = bx+1; c < bx+bw-1; c++) {
        ui95_put_at(c, by,        '\xCD', 0x1F);   /* ═ */
        ui95_put_at(c, by+bh-1,  '\xCD', 0x1F);
    }
    for (r = by+1; r < by+bh-1; r++) {
        ui95_put_at(bx,        r, '\xBA', 0x1F);   /* ║ */
        ui95_put_at(bx+bw-1,   r, '\xBA', 0x1F);
        ui95_fill_rect(bx+1, r, bw-2, 1, ' ', 0x1F);
    }

    /* ── ASCII logo ─────────────────────────────────────────────────────── */
    ui95_print_at(bx+2, by+1, "  ___   ___    _   _   _ ___    ___  ____", 0x1F);
    ui95_print_at(bx+2, by+2, " / _ | |_ _|  / \\ | | | | _ \\  / _ \\/ ___|", 0x1F);
    ui95_print_at(bx+2, by+3, "| |_| | | |  / _ \\| |_| |   / | |_| \\___ \\", 0x1F);
    ui95_print_at(bx+2, by+4, " \\__|  |___| /_/ \\_\\\\___/|_|_\\  \\___/|____/", 0x1F);

    ui95_print_at(bx+8,  by+6,  "Autonomous Intelligence OS", 0x1F);
    ui95_print_at(bx+14, by+7,  "Version  1.0.0", W95_SPLASH);

    /* Separator line */
    for (c = bx+1; c < bx+bw-1; c++)
        ui95_put_at(c, by+9, '\xC4', W95_SPLASH);

    ui95_print_at(bx+4, by+10, "Starting up, please wait...", W95_SPLASH);

    /* ── Progress bar ───────────────────────────────────────────────────── */
    int pb_x = bx + 3, pb_y = by + 12, pb_w = bw - 6;
    ui95_put_at(pb_x - 1, pb_y, '[', 0x1F);
    ui95_fill_rect(pb_x, pb_y, pb_w, 1, ' ', 0x18);   /* dark blue bg = empty */
    ui95_put_at(pb_x + pb_w, pb_y, ']', 0x1F);

    /* Animate — fill bar one block at a time */
    for (i = 0; i < pb_w; i++) {
        ui95_put_at(pb_x + i, pb_y, BOX_FILL, W95_PROGRESS);
        /* Busy-wait ~15 ms equivalent at ~100 MHz */
        volatile uint32_t d = 2500000UL; while (d--) {}
    }

    /* Brief pause */
    volatile uint32_t d = 30000000UL; while (d--) {}
}

/* ============================================================================
 * Text Input
 * ============================================================================*/

int ui95_input(int x, int y, int w, char *buf, int maxlen, int echo) {
    int len = 0;
    if (maxlen < 1) return 0;

    /* Clear the field */
    ui95_fill_rect(x, y, w, 1, ' ', W95_INPUT);
    vga_set_cursor((uint8_t)y, (uint8_t)x);

    for (;;) {
        char c = kb_getchar();

        if (c == KEY_ENTER) break;

        if (c == KEY_BACKSPACE) {
            if (len > 0) {
                len--;
                ui95_put_at(x + len, y, ' ', W95_INPUT);
                vga_set_cursor((uint8_t)y, (uint8_t)(x + len));
            }
            continue;
        }

        if (c == KEY_ESC) { buf[0] = '\0'; return 0; }

        if ((unsigned char)c >= 0x20 && (unsigned char)c < 0x80) {
            if (len < maxlen - 1 && len < w - 1) {
                buf[len] = c;
                ui95_put_at(x + len, y, echo ? c : '*', W95_INPUT);
                len++;
                vga_set_cursor((uint8_t)y, (uint8_t)(x + len));
            }
        }
    }
    buf[len] = '\0';
    return len;
}

/* ============================================================================
 * Login Screen
 * ============================================================================*/

int ui95_login_screen(char *user_out, char *pass_out, int buflen) {
    ui95_clear_desktop();
    ui95_draw_taskbar(NULL);

    /* Welcome banner across top */
    ui95_fill_rect(0, 0, VGA_COLS, 1, ' ', W95_TITLEBAR);
    ui95_print_at(2, 0, "AI Aura OS  v1.0.0  \xAE  Autonomous Intelligence Platform", W95_TITLEBAR);

    /* Login dialog centered */
    int dx = 20, dy = 5, dw = 40, dh = 13;
    ui95_draw_window(dx, dy, dw, dh, "Welcome to AI Aura OS", 1);

    /* Prompt labels */
    ui95_print_at(dx + 3, dy + 2, "Username:", W95_WINDOW);
    ui95_print_at(dx + 3, dy + 5, "Password:", W95_WINDOW);

    /* Input field outlines */
    ui95_fill_rect(dx + 3, dy + 3, dw - 7, 1, ' ', W95_INPUT);
    ui95_fill_rect(dx + 3, dy + 6, dw - 7, 1, ' ', W95_INPUT);

    /* Hint */
    ui95_print_at(dx + 3, dy + 9,  "Default:  admin / admin", W95_SHADOW);
    ui95_print_at(dx + 3, dy + 10, "Press [Enter] to confirm each field.", W95_SHADOW);

    /* Collect username */
    int n = ui95_input(dx + 3, dy + 3, dw - 7, user_out, buflen, 1);
    if (n == 0) { user_out[0] = '\0'; }

    /* Collect password (echoed as *) */
    n = ui95_input(dx + 3, dy + 6, dw - 7, pass_out, buflen, 0);
    if (n == 0) { pass_out[0] = '\0'; }

    return 1;
}

/* ============================================================================
 * Message Box
 * ============================================================================*/

void ui95_message_box(const char *title, const char *msg, uint8_t error) {
    int bx = 18, by = 8, bw = 44, bh = 7;
    uint8_t tbattr = error ? W95_ERROR : W95_TITLEBAR;

    ui95_draw_window(bx, by, bw, bh, "", 1);

    /* Override title bar colour */
    ui95_fill_rect(bx, by, bw, 1, ' ', tbattr);
    ui95_print_at(bx + 1, by, title, tbattr);
    if (bw >= 8) ui95_print_at(bx + bw - 7, by, " [_][X]", tbattr);

    /* Message text (centred vertically inside client) */
    int mlen = slen(msg);
    int mx = bx + 1 + (bw - 2 - mlen) / 2;
    if (mx < bx + 2) mx = bx + 2;
    ui95_print_at(mx, by + 2, msg, W95_WINDOW);

    /* OK button */
    ui95_print_at(bx + bw/2 - 5, by + 4, "[   OK   ]", W95_TITLEBAR);
    ui95_print_at(bx + 2, by + 5, "Press any key...", W95_SHADOW);

    kb_getchar();
}

/* ============================================================================
 * Start Menu
 * Returns the 1-based item number chosen, or 0 if ESC was pressed.
 * ============================================================================*/

#define MENU_ITEMS 9

static const char * const menu_labels[MENU_ITEMS] = {
    " System Status       ",
    " Plugin Manager      ",
    " Memory Stats        ",
    " File System         ",
    " Scheduler Tasks     ",
    " Mirror Snapshots    ",
    "\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4",  /* ─── separator */
    " About AI Aura OS    ",
    " Shut Down           ",
};

/* Which items are selectable (separators are not) */
static const uint8_t menu_selectable[MENU_ITEMS] = {1,1,1,1,1,1,0,1,1};

int ui95_start_menu(void) {
    /* Menu opens just above the taskbar at the left */
    int mx = 0, my = VGA_ROWS - 1 - MENU_ITEMS - 2;
    int mw = 23, mh = MENU_ITEMS + 2;

    /* Draw menu frame */
    ui95_fill_rect(mx, my, mw, mh, ' ', W95_WINDOW);
    /* Top and bottom border */
    int c, i;
    ui95_put_at(mx, my, BOX_TL, W95_WINDOW);
    for (c = mx+1; c < mx+mw-1; c++) ui95_put_at(c, my, BOX_H, W95_WINDOW);
    ui95_put_at(mx+mw-1, my, BOX_TR, W95_WINDOW);
    ui95_put_at(mx, my+mh-1, BOX_BL, W95_WINDOW);
    for (c = mx+1; c < mx+mw-1; c++) ui95_put_at(c, my+mh-1, BOX_H, W95_WINDOW);
    ui95_put_at(mx+mw-1, my+mh-1, BOX_BR, W95_WINDOW);

    /* Vertical "AI AURA" side strip in blue (simulates Win95 sidebar gradient) */
    for (i = 0; i < MENU_ITEMS; i++) {
        int row = my + 1 + i;
        ui95_put_at(mx, row, BOX_V, W95_WINDOW);
        ui95_put_at(mx+mw-1, row, BOX_V, W95_WINDOW);
        /* Label */
        uint8_t la = (menu_selectable[i] ? W95_WINDOW : W95_MENU_SEP);
        ui95_print_at(mx + 1, row, menu_labels[i], la);
    }

    int sel = 0;
    /* Advance initial selection to first selectable item */
    while (sel < MENU_ITEMS && !menu_selectable[sel]) sel++;

    for (;;) {
        /* Redraw items with current highlight */
        for (i = 0; i < MENU_ITEMS; i++) {
            int row = my + 1 + i;
            uint8_t attr;
            if (!menu_selectable[i])        attr = W95_MENU_SEP;
            else if (i == sel)              attr = W95_SELECTED;
            else                            attr = W95_WINDOW;
            ui95_print_at(mx + 1, row, menu_labels[i], attr);
        }

        char k = kb_getchar();

        if (k == KEY_UP) {
            int ns = sel - 1;
            while (ns >= 0 && !menu_selectable[ns]) ns--;
            if (ns >= 0) sel = ns;
        } else if (k == KEY_DOWN) {
            int ns = sel + 1;
            while (ns < MENU_ITEMS && !menu_selectable[ns]) ns++;
            if (ns < MENU_ITEMS) sel = ns;
        } else if (k == KEY_ENTER) {
            return sel + 1;   /* 1-based */
        } else if (k == KEY_ESC || k == 's' || k == 'S') {
            return 0;
        }
    }
}
