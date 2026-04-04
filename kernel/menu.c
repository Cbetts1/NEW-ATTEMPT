/* =============================================================================
 * AI Aura OS — Main Menu Implementation
 * Provides the interactive system menu driven by PS/2 keyboard polling.
 * =============================================================================*/

#include "menu.h"
#include "vga.h"
#include "plugin.h"
#include "scheduler.h"
#include "mirror.h"
#include "memory.h"
#include "kernel.h"
#include "keyboard.h"

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
    vga_println("  [R] Restore from boot snapshot");
    vga_println("  [K] Kernel panic (test)");
    vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    vga_println("  Press a key to interact...");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
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

/* -------------------------------------------------------------------------
 * menu_tick — called from the scheduler every heartbeat to handle keyboard
 * -------------------------------------------------------------------------*/
void menu_tick(void) {
    char c = keyboard_getchar();
    if (!c) return;

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);

    switch (c) {
        case '1':
            menu_show_status();
            break;
        case '2':
            plugin_list();
            break;
        case '3':
            scheduler_list();
            break;
        case '4': {
            /* Capture into the next available user slot (2..MIRROR_SLOTS-1) */
            static uint8_t snap_slot = 2;
            if (snap_slot >= MIRROR_SLOTS) snap_slot = 2;
            mirror_capture(snap_slot, "user-snap", MIRROR_FLAG_ALL);
            vga_print("[Menu] Snapshot captured in slot ");
            vga_print_dec(snap_slot);
            vga_putchar('\n');
            snap_slot++;
            break;
        }
        case '5':
            mirror_dump_all();
            break;
        case '6': {
            uint32_t used = 0, free_b = 0;
            memory_stats(&used, &free_b);
            vga_print("  Heap used : "); vga_print_dec(used);   vga_println(" bytes");
            vga_print("  Heap free : "); vga_print_dec(free_b); vga_println(" bytes");
            break;
        }
        case 'h':
        case 'H':
            vga_print("  Kernel tick : "); vga_print_dec(g_tick_count); vga_putchar('\n');
            vga_print("  Plugins     : "); vga_print_dec((uint32_t)plugin_count()); vga_putchar('\n');
            vga_print("  Tasks       : "); vga_print_dec((uint32_t)scheduler_task_count()); vga_putchar('\n');
            break;
        case 'r':
        case 'R':
            vga_println("[Menu] Restoring from boot snapshot (slot 1)...");
            mirror_restore(1);
            break;
        case 'k':
        case 'K':
            kernel_panic("User-triggered test panic from main menu.");
            break;
        default:
            /* Reprint the menu on any unrecognised key */
            menu_show_options();
            break;
    }
}
