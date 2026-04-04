/* =============================================================================
 * AI Aura OS — Main Menu Implementation
 * Provides the interactive system menu. In a full build this would read
 * keyboard scancodes; here it displays the menu and enters the kernel loop.
 * =============================================================================*/

#include "menu.h"
#include "vga.h"
#include "plugin.h"
#include "scheduler.h"
#include "mirror.h"
#include "memory.h"
#include "kernel.h"

/* Separator line */
static void draw_line(void) {
    vga_println("------------------------------------------------------------");
}

void menu_draw_banner(void) {
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    draw_line();
    vga_println("       AI AURA OS  v1.0.0   --  Aura Kernel");
    vga_println("           Autonomous Intelligence Platform");
    draw_line();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
}

static void menu_show_status(void) {
    uint32_t used = 0, free_b = 0;
    memory_stats(&used, &free_b);

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_println("\n=== System Status ===");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Memory Used : "); vga_print_dec(used);    vga_println(" bytes");
    vga_print("  Memory Free : "); vga_print_dec(free_b);  vga_println(" bytes");
    vga_print("  Plugins     : "); vga_print_dec((uint32_t)plugin_count()); vga_putchar('\n');
    vga_print("  Tasks       : "); vga_print_dec((uint32_t)scheduler_task_count()); vga_putchar('\n');

    extern volatile uint32_t g_tick_count;
    vga_print("  Kernel Tick : "); vga_print_dec(g_tick_count); vga_putchar('\n');
}

static void menu_show_options(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_println("\n=== Main Menu ===");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_println("  [1] Show system status");
    vga_println("  [2] List plugins");
    vga_println("  [3] List scheduler tasks");
    vga_println("  [4] Capture system snapshot");
    vga_println("  [5] Dump mirror snapshots");
    vga_println("  [6] Memory stats");
    vga_println("  [H] Heartbeat info");
    vga_println("  [K] Kernel panic (test)");
    vga_println("\n  (System running — kernel heartbeat active)");
}

void menu_run(void) {
    vga_clear();
    menu_draw_banner();
    menu_show_status();
    menu_show_options();

    /* Capture initial snapshot */
    mirror_capture(1, "boot-snapshot", MIRROR_FLAG_ALL);

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_println("\n[Aura] OS ready. Entering autonomous kernel loop...");
}
