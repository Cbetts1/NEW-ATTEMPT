/* =============================================================================
 * AI Aura OS — Kernel Main
 * File: kernel/kernel.c
 *
 * This is the CPU of AI Aura OS.  kernel_main() is called by entry.asm after
 * the CPU is in 32-bit protected mode.  It initialises every subsystem and
 * then drives the perpetual AI heartbeat loop.
 * =========================================================================== */
#include "include/vga.h"
#include "include/memory.h"
#include "include/event_bus.h"
#include "include/plugin.h"
#include "include/mirror.h"
#include "include/menu.h"

/* ── Forward declarations for built-in plugin descriptors ────────────────── */
extern plugin_descriptor_t aura_core_plugin;
extern plugin_descriptor_t aura_fs_adapter;
extern plugin_descriptor_t aura_net_adapter;

/* ── Heartbeat tick counter ──────────────────────────────────────────────── */
static volatile uint32_t heartbeat_ticks = 0;

/* ── Simple delay loop (roughly calibrated for ~1 GHz QEMU virtual CPU) ──── */
static void cpu_delay(uint32_t iterations)
{
    for (volatile uint32_t i = 0; i < iterations; i++) {}
}

/* ── Heartbeat event handler ─────────────────────────────────────────────── */
static void on_heartbeat(const event_t *ev)
{
    (void)ev;
    /* Keep display alive — overwrite top-right corner with a spinner */
    static const char spinner[] = "-\\|/";
    static int        spin_idx  = 0;

    uint8_t saved_color = 0;
    (void)saved_color;

    vga_set_cursor(79, 0);
    vga_set_color(VGA_COLOR(VGA_CYAN, VGA_BLACK));
    vga_putchar(spinner[spin_idx++ & 3]);
    vga_set_color(VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
}

/* ── Startup banner ──────────────────────────────────────────────────────── */
static void print_banner(void)
{
    vga_set_color(VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));
    vga_puts("##################################################\n");
    vga_puts("#                                                #\n");
    vga_puts("#          A I   A U R A   O S   v1.0           #\n");
    vga_puts("#        Self-Contained Autonomous System        #\n");
    vga_puts("#                                                #\n");
    vga_puts("##################################################\n\n");
    vga_set_color(VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));
}

/* ── kernel_main — entry point from assembly stub ────────────────────────── */
void kernel_main(void)
{
    /* 1. VGA driver — must come first (all other subsystems print to screen) */
    vga_init();
    print_banner();

    /* 2. Memory manager */
    memory_init();

    /* 3. Event bus */
    event_bus_init();
    event_subscribe(EVENT_HEARTBEAT, on_heartbeat);

    /* 4. Plugin manager + register built-in plugins */
    plugin_manager_init();
    plugin_register(&aura_core_plugin);
    plugin_register(&aura_fs_adapter);
    plugin_register(&aura_net_adapter);

    /* Load all registered plugins */
    plugin_load("aura.core");
    plugin_load("aura.fs");
    plugin_load("aura.net");

    /* 5. System mirroring engine */
    mirror_init();

    /* 6. Main menu (does not block yet — init only) */
    menu_init();

    /* Signal kernel ready */
    event_publish(EVENT_KERNEL_READY, 0, 0, "AI Aura OS kernel ready");
    event_dispatch();

    vga_puts("\n[KERN] All subsystems online. Entering heartbeat loop.\n");
    vga_puts("[KERN] Press any key to open the main menu.\n\n");

    /* ── AI Heartbeat Loop ─────────────────────────────────────────────── */
    while (1) {
        heartbeat_ticks++;

        /* Publish heartbeat event */
        event_publish(EVENT_HEARTBEAT, 0, heartbeat_ticks, (void *)0);

        /* Dispatch all queued events */
        event_dispatch();

        /* Drive plugin tick callbacks */
        plugin_tick_all();

        /* Drive mirroring engine */
        mirror_tick();

        /* Every 500 ticks, print a status line */
        if (heartbeat_ticks % 500 == 0) {
            vga_printf("[KERN] Heartbeat tick: %u\n", heartbeat_ticks);
        }

        /* Check keyboard — non-blocking poll */
        {
            /* Read PS/2 status port; 0x01 = output buffer full */
            uint8_t status;
            __asm__ volatile ("inb $0x64, %0" : "=a"(status));
            if (status & 0x01) {
                /* A key is waiting — hand control to the interactive menu */
                menu_run();
                /* menu_run() only returns if we add a "back" option later */
            }
        }

        /* Throttle loop to avoid melting the virtual CPU */
        cpu_delay(100000);
    }
    /* Never reached */
}
