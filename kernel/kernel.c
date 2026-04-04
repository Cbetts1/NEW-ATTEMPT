/* =============================================================================
 * AI Aura OS — Kernel Entry Point
 * kernel_entry is the first C function called after the bootloader jumps to
 * the kernel load address (0x10000). It sets up all subsystems, launches the
 * main menu, and enters the perpetual heartbeat loop.
 * =============================================================================*/

#include "kernel.h"
#include "vga.h"
#include "memory.h"
#include "eventbus.h"
#include "plugin.h"
#include "mirror.h"
#include "scheduler.h"
#include "menu.h"
#include "keyboard.h"
#include "../env/env.h"
#include "../env/fs.h"
#include "../modules/loader.h"
#include "../adapters/adapter.h"

/* -------------------------------------------------------------------------
 * Global kernel state
 * -------------------------------------------------------------------------*/
volatile kernel_state_t g_kernel_state = KERNEL_STATE_BOOT;
volatile uint32_t       g_tick_count   = 0;

/* -------------------------------------------------------------------------
 * Heartbeat — called every main-loop iteration
 * -------------------------------------------------------------------------*/
void kernel_heartbeat(void) {
    g_tick_count++;
}

/* -------------------------------------------------------------------------
 * Panic — halt the system with a message
 * -------------------------------------------------------------------------*/
void kernel_panic(const char *msg) {
    g_kernel_state = KERNEL_STATE_PANIC;
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_println("\n*** KERNEL PANIC ***");
    if (msg) vga_println(msg);
    vga_println("System halted.");
    __asm__ volatile ("cli; hlt");
    while (1) {} /* Should never reach here */
}

/* -------------------------------------------------------------------------
 * Kernel entry — called by the bootloader
 * -------------------------------------------------------------------------*/
void kernel_main(void) {
    g_kernel_state = KERNEL_STATE_INIT;

    /* 1. VGA driver must come first — all other subsystems may print */
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_println("AI Aura OS — Kernel Initializing");
    vga_println("==================================");

    /* 2. Memory manager */
    memory_init();
    vga_println("[OK] Memory Manager initialized");

    /* 3. Event bus */
    eventbus_init();
    vga_println("[OK] Event Bus initialized");

    /* 4. Plugin manager */
    plugin_manager_init();
    vga_println("[OK] Plugin Manager initialized");

    /* 5. System mirroring engine */
    mirror_init();
    vga_println("[OK] System Mirror initialized");

    /* 6. Task scheduler */
    scheduler_init();
    vga_println("[OK] Scheduler initialized");

    /* 7. Environment registry and virtual filesystem */
    env_init();
    vga_println("[OK] Environment registry initialized");
    fs_init();
    vga_println("[OK] Virtual filesystem initialized");

    /* 8. Keyboard driver */
    keyboard_init();
    vga_println("[OK] Keyboard driver initialized");

    /* 9. Register built-in kernel tasks */
    scheduler_add_task("heartbeat",    kernel_heartbeat,  1);
    scheduler_add_task("event_drain",  eventbus_process,  1);
    scheduler_add_task("mirror_sync",  mirror_sync,      10);
    scheduler_add_task("plugin_tick",  plugin_tick_all,   5);
    scheduler_add_task("keyboard_poll",keyboard_poll,     1);
    scheduler_add_task("menu_tick",    menu_tick,         2);
    scheduler_add_task("serial_poll",  serial_poll,       5);

    vga_println("[OK] Built-in tasks registered");

    /* 10. Load built-in modules (hello, aura_core) */
    loader_init();
    loader_load_all();
    vga_println("[OK] Modules loaded");

    /* 11. Register hardware adapters */
    adapter_serial_register();
    adapter_net_register();
    vga_println("[OK] Adapters registered");

    vga_println("==================================");

    /* 8. Transition to running state and show main menu */
    g_kernel_state = KERNEL_STATE_RUN;
    menu_run();

    /* -----------------------------------------------------------------------
     * Perpetual Kernel Heartbeat Loop
     * Everything in AI Aura OS is driven from this loop via the scheduler.
     * ----------------------------------------------------------------------- */
    while (1) {
        scheduler_tick();
    }

    /* Unreachable — OS is autonomous and never exits */
}
