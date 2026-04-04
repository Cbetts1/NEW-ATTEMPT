#ifndef UI95_H
#define UI95_H

/* =============================================================================
 * AI Aura OS — Windows 95–Themed VGA Text UI
 *
 * Implements a Windows 95–style interface using 80×25 VGA text mode.
 * Colors use the 4-bit VGA attribute byte (bg<<4 | fg).
 *
 * Color reference (VGA text mode):
 *   Background can use 0–7 only (bit 7 = blink in default mode).
 *   Foreground can use 0–15.
 *   Attribute byte = (bg << 4) | fg
 *
 * Win95 palette choices used here:
 *   W95_DESKTOP  = 0x70  light-gray bg,  black fg        (desktop)
 *   W95_TITLEBAR = 0x1F  blue bg,        bright-white fg (active title)
 *   W95_INACTIVE = 0x78  light-gray bg,  dark-gray fg    (inactive title)
 *   W95_WINDOW   = 0x70  light-gray bg,  black fg        (client area)
 *   W95_TASKBAR  = 0x70  light-gray bg,  black fg        (taskbar strip)
 *   W95_SELECTED = 0x1F  blue bg,        bright-white fg (menu selection)
 *   W95_INPUT    = 0x07  black bg,       light-gray fg   (text input box)
 *   W95_SHADOW   = 0x78  light-gray bg,  dark-gray fg    (3-D shadow edge)
 *   W95_ERROR    = 0x4F  red bg,         bright-white fg (error title bar)
 *   W95_SPLASH   = 0x17  blue bg,        light-gray fg   (boot splash)
 *   W95_PROGRESS = 0x2A  green bg,       bright-green fg (progress fill)
 *   W95_START    = 0x1F  blue bg,        bright-white fg ([Start] button)
 * =============================================================================*/

#include <stdint.h>

#define VGA_COLS 80
#define VGA_ROWS 25

#define W95_DESKTOP   0x70
#define W95_TITLEBAR  0x1F
#define W95_INACTIVE  0x78
#define W95_WINDOW    0x70
#define W95_TASKBAR   0x70
#define W95_SELECTED  0x1F
#define W95_INPUT     0x07
#define W95_SHADOW    0x78
#define W95_ERROR     0x4F
#define W95_SPLASH    0x17
#define W95_PROGRESS  0x2A
#define W95_START     0x1F
#define W95_MENU_SEP  0x78

/* IBM PC-8 / OEM-437 box-drawing chars used for window frames */
#define BOX_TL   '\xDA'   /* ┌ */
#define BOX_TR   '\xBF'   /* ┐ */
#define BOX_BL   '\xC0'   /* └ */
#define BOX_BR   '\xD9'   /* ┘ */
#define BOX_H    '\xC4'   /* ─ */
#define BOX_V    '\xB3'   /* │ */
#define BOX_FILL '\xDB'   /* █ full block (progress bar) */

/* ---------------------------------------------------------------------------
 * Low-level positional drawing
 * ---------------------------------------------------------------------------*/
void ui95_put_at(int x, int y, char c, uint8_t attr);
void ui95_print_at(int x, int y, const char *s, uint8_t attr);
void ui95_print_dec_at(int x, int y, uint32_t val, uint8_t attr);
void ui95_fill_rect(int x, int y, int w, int h, char c, uint8_t attr);

/* ---------------------------------------------------------------------------
 * Widgets
 * ---------------------------------------------------------------------------*/
/* Draw a titled window frame; client rows are y+1 … y+h-2 */
void ui95_draw_window(int x, int y, int w, int h,
                      const char *title, uint8_t active);

/* Taskbar strip at row 24 */
void ui95_draw_taskbar(const char *open_app);

/* Clear the 24 desktop rows to the desktop color */
void ui95_clear_desktop(void);

/* ---------------------------------------------------------------------------
 * High-level screens
 * ---------------------------------------------------------------------------*/

/* Animated boot splash (blocks until animation completes) */
void ui95_startup_splash(void);

/* Login dialog. Fills user_out/pass_out (buflen each). Returns always 1. */
int  ui95_login_screen(char *user_out, char *pass_out, int buflen);

/* Inline text input at (x,y) width w.
 * echo=1: show typed chars; echo=0: echo '*'.
 * Returns number of chars entered. */
int  ui95_input(int x, int y, int w, char *buf, int maxlen, int echo);

/* Modal message box. error=1 → red title bar, error=0 → blue. */
void ui95_message_box(const char *title, const char *msg, uint8_t error);

/* Pop-up start menu. Returns 1-based choice, or 0 if cancelled (ESC). */
int  ui95_start_menu(void);

#endif /* UI95_H */
