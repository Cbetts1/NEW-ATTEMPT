/* =============================================================================
 * AI Aura OS — VGA Driver Stub for Host-Side Tests
 *
 * All VGA functions are no-ops so that kernel modules that call vga_print*
 * can be unit-tested on a host machine without the 0xB8000 framebuffer.
 * =============================================================================*/

#include "vga.h"

void vga_init(void)                               {}
void vga_clear(void)                              {}
void vga_set_color(vga_color_t fg, vga_color_t bg) { (void)fg; (void)bg; }
void vga_putchar(char c)                          { (void)c; }
void vga_print(const char *str)                   { (void)str; }
void vga_println(const char *str)                 { (void)str; }
void vga_print_hex(uint32_t val)                  { (void)val; }
void vga_print_dec(uint32_t val)                  { (void)val; }
void vga_set_cursor(uint8_t row, uint8_t col)     { (void)row; (void)col; }
