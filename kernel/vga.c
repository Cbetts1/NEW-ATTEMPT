/* =============================================================================
 * AI Aura OS — VGA Text-Mode Driver
 * File: kernel/vga.c
 * =========================================================================== */
#include "include/vga.h"
#include <stdarg.h>
#include <stdint.h>

/* VGA text buffer lives at physical address 0xB8000 */
static volatile uint16_t * const VGA_BUF = (volatile uint16_t *)0xB8000;

static int     vga_col   = 0;
static int     vga_row   = 0;
static uint8_t vga_color = 0;

/* ── helpers ────────────────────────────────────────────────────────────────*/

static inline uint16_t vga_entry(char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void vga_scroll(void)
{
    for (int r = 1; r < VGA_ROWS; r++)
        for (int c = 0; c < VGA_COLS; c++)
            VGA_BUF[(r - 1) * VGA_COLS + c] = VGA_BUF[r * VGA_COLS + c];

    for (int c = 0; c < VGA_COLS; c++)
        VGA_BUF[(VGA_ROWS - 1) * VGA_COLS + c] = vga_entry(' ', vga_color);

    vga_row = VGA_ROWS - 1;
}

/* ── public API ─────────────────────────────────────────────────────────────*/

void vga_init(void)
{
    vga_color = VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_clear();
}

void vga_clear(void)
{
    for (int r = 0; r < VGA_ROWS; r++)
        for (int c = 0; c < VGA_COLS; c++)
            VGA_BUF[r * VGA_COLS + c] = vga_entry(' ', vga_color);
    vga_col = 0;
    vga_row = 0;
}

void vga_set_color(uint8_t color)
{
    vga_color = color;
}

void vga_set_cursor(int col, int row)
{
    vga_col = col;
    vga_row = row;
}

void vga_putchar(char c)
{
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_ROWS)
            vga_scroll();
        return;
    }
    if (c == '\r') {
        vga_col = 0;
        return;
    }
    if (c == '\t') {
        vga_col = (vga_col + 8) & ~7;
        if (vga_col >= VGA_COLS) {
            vga_col = 0;
            vga_row++;
            if (vga_row >= VGA_ROWS)
                vga_scroll();
        }
        return;
    }

    VGA_BUF[vga_row * VGA_COLS + vga_col] = vga_entry(c, vga_color);
    vga_col++;
    if (vga_col >= VGA_COLS) {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_ROWS)
            vga_scroll();
    }
}

void vga_puts(const char *str)
{
    while (*str)
        vga_putchar(*str++);
}

/* ── minimal printf (supports %s %d %u %x %c %%) ──────────────────────────*/

static void print_uint(unsigned int n, int base)
{
    static const char digits[] = "0123456789ABCDEF";
    char buf[16];
    int  i = 0;
    if (n == 0) { vga_putchar('0'); return; }
    while (n) {
        buf[i++] = digits[n % (unsigned)base];
        n /= (unsigned)base;
    }
    while (i--) vga_putchar(buf[i]);
}

static void print_int(int n)
{
    if (n < 0) { vga_putchar('-'); n = -n; }
    print_uint((unsigned int)n, 10);
}

void vga_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    for (; *fmt; fmt++) {
        if (*fmt != '%') { vga_putchar(*fmt); continue; }
        fmt++;
        switch (*fmt) {
            case 's': vga_puts(va_arg(ap, const char *)); break;
            case 'd': print_int(va_arg(ap, int));         break;
            case 'u': print_uint(va_arg(ap, unsigned int), 10); break;
            case 'x': print_uint(va_arg(ap, unsigned int), 16); break;
            case 'c': vga_putchar((char)va_arg(ap, int)); break;
            case '%': vga_putchar('%');                   break;
            default:  vga_putchar('%'); vga_putchar(*fmt); break;
        }
    }

    va_end(ap);
}
