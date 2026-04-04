/* =============================================================================
 * AI Aura OS — Main Menu
 * File: kernel/menu.c
 *
 * Interactive text-mode main menu.  Reads keystrokes via port I/O (PS/2
 * keyboard status register) and dispatches to registered actions.
 * =========================================================================== */
#include "include/menu.h"
#include "include/vga.h"
#include "include/plugin.h"
#include "include/mirror.h"
#include "include/memory.h"
#include "include/event_bus.h"

/* ── minimal port I/O ────────────────────────────────────────────────────── */
static inline uint8_t inb(uint16_t port)
{
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

#define KBD_STATUS  0x64
#define KBD_DATA    0x60

/* Scancode → ASCII (very partial, US layout, unshifted) */
static const char sc_map[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', 8,
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' '
};

static char kbd_getchar(void)
{
    /* poll until a key is available */
    while (!(inb(KBD_STATUS) & 0x01)) {}
    uint8_t sc = inb(KBD_DATA);
    if (sc & 0x80) return 0;            /* key-release — ignore */
    if (sc < 128)  return sc_map[sc];
    return 0;
}

/* ── menu actions ────────────────────────────────────────────────────────── */

static void action_plugin_list(void)
{
    vga_puts("\n");
    plugin_list();
}

static void action_mirror_dump(void)
{
    vga_puts("\n");
    mirror_dump();
}

static void action_memory_dump(void)
{
    vga_puts("\n");
    memory_dump();
}

static void action_capture_snapshot(void)
{
    mirror_capture(MIRROR_SUBSYS_ALL);
    vga_puts("\nSnapshot captured.\n");
}

static void action_shutdown(void)
{
    vga_puts("\nShutting down AI Aura OS...\n");
    event_publish(EVENT_SHUTDOWN, 0, 0, "user requested");
    event_dispatch();
    __asm__ volatile ("cli; hlt");
}

/* ── menu table ──────────────────────────────────────────────────────────── */

static menu_entry_t entries[] = {
    { "1. List plugins / adapters",  action_plugin_list    },
    { "2. System mirror dump",       action_mirror_dump    },
    { "3. Memory heap dump",         action_memory_dump    },
    { "4. Capture snapshot now",     action_capture_snapshot },
    { "5. Shutdown",                 action_shutdown       },
};

#define ENTRY_COUNT ((int)(sizeof(entries)/sizeof(entries[0])))

/* ── Public API ──────────────────────────────────────────────────────────── */

void menu_init(void)
{
    vga_puts("[MNU] Main menu initialised.\n");
}

void menu_draw(void)
{
    vga_set_color(VGA_COLOR(VGA_YELLOW, VGA_BLACK));
    vga_puts("\n============================================\n");
    vga_puts("  AI AURA OS  —  Main Menu\n");
    vga_puts("============================================\n");
    vga_set_color(VGA_COLOR(VGA_LIGHT_GREEN, VGA_BLACK));

    for (int i = 0; i < ENTRY_COUNT; i++) {
        vga_puts("  ");
        vga_puts(entries[i].label);
        vga_puts("\n");
    }

    vga_set_color(VGA_COLOR(VGA_WHITE, VGA_BLACK));
    vga_puts("\nPress key [1-5]: ");
}

void menu_run(void)
{
    while (1) {
        menu_draw();
        char c = 0;
        while (!c)
            c = kbd_getchar();

        int choice = c - '1';
        if (choice >= 0 && choice < ENTRY_COUNT && entries[choice].action)
            entries[choice].action();

        /* small pause so output is readable before redrawing */
        for (volatile int i = 0; i < 5000000; i++) {}
    }
}
