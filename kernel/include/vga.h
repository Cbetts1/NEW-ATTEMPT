/* =============================================================================
 * AI Aura OS — VGA text-mode I/O header
 * File: kernel/include/vga.h
 * =========================================================================== */
#ifndef AIOS_VGA_H
#define AIOS_VGA_H

#include <stdint.h>

/* VGA colour byte (foreground | background << 4) */
#define VGA_COLOR(fg, bg)   ((uint8_t)((bg) << 4 | (fg)))

/* Foreground / background colour indices */
#define VGA_BLACK           0
#define VGA_BLUE            1
#define VGA_GREEN           2
#define VGA_CYAN            3
#define VGA_RED             4
#define VGA_MAGENTA         5
#define VGA_BROWN           6
#define VGA_LIGHT_GREY      7
#define VGA_DARK_GREY       8
#define VGA_LIGHT_BLUE      9
#define VGA_LIGHT_GREEN     10
#define VGA_LIGHT_CYAN      11
#define VGA_LIGHT_RED       12
#define VGA_LIGHT_MAGENTA   13
#define VGA_YELLOW          14
#define VGA_WHITE           15

/* VGA screen geometry */
#define VGA_COLS            80
#define VGA_ROWS            25

void vga_init(void);
void vga_clear(void);
void vga_set_color(uint8_t color);
void vga_putchar(char c);
void vga_puts(const char *str);
void vga_printf(const char *fmt, ...);
void vga_set_cursor(int col, int row);

#endif /* AIOS_VGA_H */
