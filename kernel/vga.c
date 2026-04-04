/* =============================================================================
 * AI Aura OS — VGA Text-Mode Driver Implementation
 * =============================================================================*/

#include "vga.h"
#include "io.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_BUFFER  ((volatile uint16_t *)0xB8000)

/* VGA cursor and color state */
static uint8_t  vga_row;
static uint8_t  vga_col;
static uint8_t  vga_color;

/* -------------------------------------------------------------------------- */

static inline uint8_t vga_make_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t)((bg << 4) | (fg & 0x0F));
}

static inline uint16_t vga_make_entry(char c, uint8_t color) {
    return (uint16_t)((uint16_t)(uint8_t)c | ((uint16_t)color << 8));
}

static void vga_update_cursor(void) {
    uint16_t pos = (uint16_t)(vga_row * VGA_WIDTH + vga_col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void vga_scroll(void) {
    /* Move all rows up by one */
    for (uint8_t row = 0; row < VGA_HEIGHT - 1; row++) {
        for (uint8_t col = 0; col < VGA_WIDTH; col++) {
            VGA_BUFFER[row * VGA_WIDTH + col] =
                VGA_BUFFER[(row + 1) * VGA_WIDTH + col];
        }
    }
    /* Clear the last row */
    for (uint8_t col = 0; col < VGA_WIDTH; col++) {
        VGA_BUFFER[(VGA_HEIGHT - 1) * VGA_WIDTH + col] =
            vga_make_entry(' ', vga_color);
    }
    if (vga_row > 0) {
        vga_row = (uint8_t)(VGA_HEIGHT - 1);
    }
}

/* -------------------------------------------------------------------------- */

void vga_init(void) {
    vga_row   = 0;
    vga_col   = 0;
    vga_color = vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_clear(void) {
    for (uint8_t row = 0; row < VGA_HEIGHT; row++) {
        for (uint8_t col = 0; col < VGA_WIDTH; col++) {
            VGA_BUFFER[row * VGA_WIDTH + col] =
                vga_make_entry(' ', vga_color);
        }
    }
    vga_row = 0;
    vga_col = 0;
    vga_update_cursor();
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_color = vga_make_color(fg, bg);
}

void vga_set_cursor(uint8_t row, uint8_t col) {
    vga_row = row;
    vga_col = col;
    vga_update_cursor();
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (uint8_t)((vga_col + 8) & ~7);
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
        }
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
            VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] =
                vga_make_entry(' ', vga_color);
        }
    } else {
        VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] =
            vga_make_entry(c, vga_color);
        vga_col++;
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
        }
    }

    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
    }
    vga_update_cursor();
}

void vga_print(const char *str) {
    if (!str) return;
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_println(const char *str) {
    vga_print(str);
    vga_putchar('\n');
}

void vga_print_hex(uint32_t val) {
    const char hex[] = "0123456789ABCDEF";
    vga_print("0x");
    for (int i = 28; i >= 0; i -= 4) {
        vga_putchar(hex[(val >> i) & 0xF]);
    }
}

void vga_print_dec(uint32_t val) {
    if (val == 0) {
        vga_putchar('0');
        return;
    }
    char buf[12];
    int  i = 0;
    while (val > 0) {
        buf[i++] = (char)('0' + (val % 10));
        val /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        vga_putchar(buf[j]);
    }
}
