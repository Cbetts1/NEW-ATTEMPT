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
#include "idt.h"
#include "pit.h"
#include "paging.h"
#include "../env/env.h"
#include "../env/fs.h"
#include "../env/fat12.h"
#include "../modules/loader.h"
#include "../adapters/adapter.h"
#include "../adapters/ata.h"

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
    /* Attempt to persist env before halting */
    env_save();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_println("\n*** KERNEL PANIC ***");
    if (msg) vga_println(msg);
    vga_println("System halted.");
    __asm__ volatile ("cli; hlt");
    while (1) {} /* Should never reach here */
}

/* -------------------------------------------------------------------------
 * Clean shutdown — persist state and halt
 * -------------------------------------------------------------------------*/
void kernel_shutdown(void) {
    g_kernel_state = KERNEL_STATE_PANIC; /* Re-use PANIC state to stop scheduler */
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_println("\n[Shutdown] Saving environment...");
    if (env_save() == 0) {
        vga_println("[Shutdown] Environment saved to AIOS.ENV.");
    } else {
        vga_println("[Shutdown] Save skipped (no disk or FAT12 error).");
    }
    vga_println("[Shutdown] AI Aura OS halted. Safe to power off.");
    __asm__ volatile ("cli; hlt");
    while (1) {}
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

    /* 3. Enable paging: identity-map first 4 MB with heap guard pages */
    paging_init();

    /* 4. Event bus */
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

    /* 8. IDT + PIC remapping — must be done before enabling any IRQ drivers */
    idt_init();
    vga_println("[OK] IDT + PIC initialized");

    /* 9. PIT timer at PIT_FREQUENCY_HZ (~100 Hz) — drives g_tick_count via IRQ0 */
    pit_init();
    vga_println("[OK] PIT timer initialized");

    /* 10. Keyboard driver — registers IRQ1 handler after IDT is ready */
    keyboard_init();
    vga_println("[OK] Keyboard driver initialized");

    /* 11. Register built-in kernel tasks.
     * keyboard_poll is still registered as a fallback polling task in case
     * the IRQ1 handler misses a scancode (e.g. early boot before IDT ready). */
    scheduler_add_task("heartbeat",    kernel_heartbeat,  1);
    scheduler_add_task("event_drain",  eventbus_process,  1);
    scheduler_add_task("mirror_sync",  mirror_sync,      10);
    scheduler_add_task("plugin_tick",  plugin_tick_all,   5);
    scheduler_add_task("keyboard_poll",keyboard_poll,     1);
    scheduler_add_task("menu_tick",    menu_tick,         2);
    scheduler_add_task("serial_poll",  serial_poll,       5);

    vga_println("[OK] Built-in tasks registered");

    /* 12. Load built-in modules (hello, aura_core) */
    loader_init();
    loader_load_all();
    vga_println("[OK] Modules loaded");

    /* 13. Register hardware adapters */
    adapter_serial_register();
    adapter_net_register();
    vga_println("[OK] Adapters registered");

    /* 14. ATA disk driver + FAT12 filesystem */
    if (ata_init() == AURA_OK) {
        vga_println("[OK] ATA primary master detected");
        if (fat12_init() == AURA_OK) {
            vga_println("[OK] FAT12 volume mounted");
            /* Overlay env from persisted AIOS.ENV if it exists */
            if (env_load() == 0) {
                vga_println("[OK] Environment loaded from AIOS.ENV");
            }
        } else {
            vga_println("[  ] FAT12 mount skipped (no FAT12 volume)");
        }
    } else {
        vga_println("[  ] ATA drive not present — disk I/O disabled");
    }

    vga_println("==================================");

    /* 14. Transition to running state and show main menu */
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
