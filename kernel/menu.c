/* =============================================================================
 * AI Aura OS — Main Menu (Windows 95–Themed Interactive Desktop)
 *
 * Flow:
 *   1. ui95_startup_splash()     — animated boot logo
 *   2. Login loop                — up to 3 authentication attempts
 *   3. desktop_loop()            — taskbar + Start-menu driven OS shell
 *
 * The desktop_loop() runs the kernel scheduler on every iteration so all
 * background tasks (event bus, mirror sync, plugin ticks) continue to fire.
 * =============================================================================*/

#include "menu.h"
#include "ui95.h"
#include "keyboard.h"
#include "user.h"
#include "vga.h"
#include "plugin.h"
#include "scheduler.h"
#include "mirror.h"
#include "memory.h"
#include "kernel.h"
#include "fs.h"
#include "env.h"

extern volatile uint32_t g_tick_count;

/* ============================================================================
 * Helper: number → string in-place
 * ============================================================================*/
static int u32_to_str(uint32_t v, char *buf, int buflen) {
    char tmp[12]; int n = 0, i;
    if (v == 0) { buf[0]='0'; buf[1]='\0'; return 1; }
    while (v > 0 && n < 12) { tmp[n++] = (char)('0' + v % 10); v /= 10; }
    if (n >= buflen) n = buflen - 1;
    for (i = 0; i < n; i++) buf[i] = tmp[n - 1 - i];
    buf[n] = '\0';
    return n;
}

/* Print label + decimal value at (x,y) */
static void kv_line(int x, int y, const char *label, uint32_t val,
                    const char *suffix, uint8_t attr) {
    char num[12];
    u32_to_str(val, num, sizeof(num));
    ui95_print_at(x, y, label, attr);
    int lx = x;
    while (label[lx - x]) lx++;      /* skip past label end */
    lx = x + (int)(sizeof("") - 1);  /* re-anchor */
    /* simpler: print fields separately */
    (void)lx;
    /* print label */
    ui95_print_at(x, y, label, attr);
    /* print number right after label */
    int ln = 0; while (label[ln]) ln++;
    ui95_print_at(x + ln, y, num, attr);
    /* suffix */
    int nn = 0; while (num[nn]) nn++;
    ui95_print_at(x + ln + nn, y, suffix, attr);
}

/* ============================================================================
 * Info windows (opened from the Start menu)
 * ============================================================================*/

static void win_system_status(void) {
    int wx = 4, wy = 1, ww = 72, wh = 18;
    ui95_draw_window(wx, wy, ww, wh, "System Status", 1);

    char nbuf[12];
    int row = wy + 2, cx = wx + 3;

    ui95_print_at(cx, row++, "AI Aura OS  v1.0.0  \xAE  Autonomous Intelligence Platform", W95_WINDOW);
    row++;

    /* Memory */
    ui95_print_at(cx, row, "MEMORY", W95_TITLEBAR);
    ui95_fill_rect(cx + 6, row, ww - 10, 1, '\xC4', W95_SHADOW); row++;
    uint32_t used = 0, free_b = 0;
    memory_stats(&used, &free_b);
    kv_line(cx + 2, row++, "  Used  : ", used,   " bytes", W95_WINDOW);
    kv_line(cx + 2, row++, "  Free  : ", free_b, " bytes", W95_WINDOW);
    kv_line(cx + 2, row++, "  Heap  : 0x", 0x00200000UL, "", W95_WINDOW);
    row++;

    /* Scheduler */
    ui95_print_at(cx, row, "SCHEDULER", W95_TITLEBAR);
    ui95_fill_rect(cx + 9, row, ww - 13, 1, '\xC4', W95_SHADOW); row++;
    kv_line(cx + 2, row++, "  Tasks : ", (uint32_t)scheduler_task_count(), "", W95_WINDOW);
    kv_line(cx + 2, row++, "  Ticks : ", g_tick_count, "", W95_WINDOW);
    row++;

    /* Plugins */
    ui95_print_at(cx, row, "PLUGINS", W95_TITLEBAR);
    ui95_fill_rect(cx + 7, row, ww - 11, 1, '\xC4', W95_SHADOW); row++;
    kv_line(cx + 2, row++, "  Active: ", (uint32_t)plugin_count(), "", W95_WINDOW);
    row++;

    /* Environment */
    ui95_print_at(cx, row, "ENVIRONMENT", W95_TITLEBAR);
    ui95_fill_rect(cx + 11, row, ww - 15, 1, '\xC4', W95_SHADOW); row++;
    const char *os_name = env_get("OS_NAME");
    const char *os_ver  = env_get("OS_VERSION");
    ui95_print_at(cx + 2, row++, "  OS_NAME    : ", W95_WINDOW);
    ui95_print_at(cx + 17, row - 1, os_name ? os_name : "N/A", W95_WINDOW);
    ui95_print_at(cx + 2, row++, "  OS_VERSION : ", W95_WINDOW);
    ui95_print_at(cx + 17, row - 1, os_ver  ? os_ver  : "N/A", W95_WINDOW);

    ui95_print_at(cx, wh + wy - 2, "  Press any key to close...", W95_SHADOW);
    kb_getchar();
    (void)nbuf;
}

static void win_plugin_manager(void) {
    int wx = 4, wy = 1, ww = 72, wh = 20;
    ui95_draw_window(wx, wy, ww, wh, "Plugin Manager", 1);
    ui95_print_at(wx + 3, wy + 2, "Registered plugins:", W95_TITLEBAR);
    plugin_list();   /* prints via vga_print — cursor is inside window */
    ui95_print_at(wx + 3, wy + wh - 2, "  Press any key...", W95_SHADOW);
    kb_getchar();
}

static void win_memory_stats(void) {
    int wx = 4, wy = 1, ww = 72, wh = 10;
    ui95_draw_window(wx, wy, ww, wh, "Memory Stats", 1);
    uint32_t used = 0, free_b = 0;
    memory_stats(&used, &free_b);
    kv_line(wx + 3, wy + 2, "  Heap base   : 0x00200000  size : 0x", 0x00080000UL, " (512 KB)", W95_WINDOW);
    kv_line(wx + 3, wy + 4, "  Used bytes  : ", used,   "", W95_WINDOW);
    kv_line(wx + 3, wy + 5, "  Free bytes  : ", free_b, "", W95_WINDOW);
    kv_line(wx + 3, wy + 6, "  Kernel tick : ", g_tick_count, "", W95_WINDOW);
    ui95_print_at(wx + 3, wy + wh - 2, "  Press any key...", W95_SHADOW);
    kb_getchar();
}

static void win_filesystem(void) {
    int wx = 4, wy = 1, ww = 72, wh = 20;
    ui95_draw_window(wx, wy, ww, wh, "Virtual Filesystem", 1);
    int row = wy + 2;
    ui95_print_at(wx + 3, row++, "  Name                  Type   Size", W95_TITLEBAR);
    ui95_print_at(wx + 3, row++, "  ─────────────────────────────────────────────", W95_SHADOW);

    /* Walk the FS table via public API */
    extern fs_node_t *fs_find(const char *name);
    /* Iterate known names by trying to look up common paths */
    static const char * const paths[] = {"/", "/README", NULL};
    int i;
    for (i = 0; paths[i]; i++) {
        fs_node_t *n = fs_find(paths[i]);
        if (!n || row >= wy + wh - 2) continue;
        ui95_print_at(wx + 5, row, n->name, W95_WINDOW);
        ui95_print_at(wx + 29, row,
                      (n->type == FS_TYPE_DIR) ? "<DIR>" : "FILE", W95_WINDOW);
        if (n->type != FS_TYPE_DIR)
            ui95_print_dec_at(wx + 37, row, n->size, W95_WINDOW);
        row++;
    }

    ui95_print_at(wx + 3, wy + wh - 2, "  Press any key...", W95_SHADOW);
    kb_getchar();
}

static void win_scheduler(void) {
    int wx = 4, wy = 1, ww = 72, wh = 12;
    ui95_draw_window(wx, wy, ww, wh, "Scheduler Tasks", 1);
    vga_set_cursor((uint8_t)(wy + 2), (uint8_t)(wx + 3));
    scheduler_list();
    ui95_print_at(wx + 3, wy + wh - 2, "  Press any key...", W95_SHADOW);
    kb_getchar();
}

static void win_mirror(void) {
    int wx = 4, wy = 1, ww = 72, wh = 12;
    ui95_draw_window(wx, wy, ww, wh, "Mirror Snapshots", 1);
    vga_set_cursor((uint8_t)(wy + 2), (uint8_t)(wx + 3));
    mirror_dump_all();
    ui95_print_at(wx + 3, wy + wh - 2, "  Press any key...", W95_SHADOW);
    kb_getchar();
}

static void win_about(void) {
    int wx = 14, wy = 3, ww = 52, wh = 17;
    ui95_draw_window(wx, wy, ww, wh, "About AI Aura OS", 1);

    int cx = wx + 4;
    ui95_print_at(cx, wy + 2,  "  AI Aura OS  v1.0.0",          W95_TITLEBAR);
    ui95_print_at(cx, wy + 3,  "  Autonomous Intelligence OS",   W95_WINDOW);
    ui95_print_at(cx, wy + 5,  "  Architecture : x86 (i386)",    W95_WINDOW);
    ui95_print_at(cx, wy + 6,  "  Mode         : 32-bit protected", W95_WINDOW);
    ui95_print_at(cx, wy + 7,  "  Heap         : 512 KB @ 2 MB", W95_WINDOW);
    ui95_print_at(cx, wy + 8,  "  Display      : VGA 80x25 text", W95_WINDOW);
    ui95_print_at(cx, wy + 9,  "  Kernel type  : Cooperative", W95_WINDOW);
    ui95_print_at(cx, wy + 10, "  Build host   : Termux / Linux", W95_WINDOW);
    ui95_print_at(cx, wy + 12, "  Theme        : Windows 95 (inspired)", W95_SHADOW);
    ui95_print_at(cx, wy + 13, "  License      : See LICENSE file", W95_SHADOW);
    ui95_print_at(cx, wy + wh - 2, "  Press any key...", W95_SHADOW);
    kb_getchar();
}

/* ============================================================================
 * Shutdown confirmation
 * ============================================================================*/
static int win_shutdown(void) {
    ui95_message_box("Shut Down AI Aura OS",
                     "Are you sure?  Press ENTER to halt, ESC to cancel.",
                     0);
    /* message_box already consumed a key; here we just return 1 to halt */
    return 1;
}

/* ============================================================================
 * Desktop (main kernel loop)
 * ============================================================================*/

static void draw_desktop_base(const char *username) {
    ui95_clear_desktop();

    /* Header bar */
    ui95_fill_rect(0, 0, VGA_COLS, 1, ' ', W95_TITLEBAR);
    ui95_print_at(2, 0, "AI Aura OS  v1.0.0", W95_TITLEBAR);
    ui95_print_at(VGA_COLS - 18, 0, "User: ", W95_TITLEBAR);
    ui95_print_at(VGA_COLS - 12, 0, username, W95_TITLEBAR);

    /* Desktop hint */
    ui95_print_at(2,  12, "Press  [S]  to open the Start Menu", W95_SHADOW);
    ui95_print_at(2,  13, "Use  [\x18][\x19]  arrows to navigate, [Enter] to select", W95_SHADOW);
    ui95_print_at(2,  14, "Press  [ESC]  inside Start Menu to close it", W95_SHADOW);

    /* Aura OS icon (text art) */
    ui95_print_at(33,  4, "\xDB\xDB\xDB\xDB\xDB\xDB", 0x1A);
    ui95_print_at(33,  5, "\xDB      \xDB", 0x1A);
    ui95_print_at(33,  6, "\xDB  AI  \xDB", 0x1A);
    ui95_print_at(33,  7, "\xDB      \xDB", 0x1A);
    ui95_print_at(33,  8, "\xDB\xDB\xDB\xDB\xDB\xDB", 0x1A);
    ui95_print_at(32,  9, "AI Aura OS", W95_SHADOW);

    ui95_draw_taskbar("AI Aura OS");
}

static void desktop_loop(const char *username) {
    draw_desktop_base(username);

    for (;;) {
        /* Run one scheduler tick on every loop pass */
        scheduler_tick();

        /* Poll for keyboard input — non-blocking */
        if (!kb_haskey()) continue;

        char k = kb_getchar();

        if (k == 's' || k == 'S') {
            /* Open Start Menu */
            int choice = ui95_start_menu();

            /* Restore desktop after menu closes */
            draw_desktop_base(username);

            switch (choice) {
                case 1: win_system_status(); break;
                case 2: win_plugin_manager(); break;
                case 3: win_memory_stats();   break;
                case 4: win_filesystem();     break;
                case 5: win_scheduler();      break;
                case 6: win_mirror();         break;
                /* 7 = separator, skip */
                case 8: win_about();          break;
                case 9:
                    win_shutdown();
                    kernel_panic("User-initiated shutdown.");
                    break;
                default: break;
            }

            /* Restore desktop after any window closes */
            draw_desktop_base(username);
        }
        /* Ignore all other keys on the bare desktop */
    }
}

/* ============================================================================
 * Public entry points
 * ============================================================================*/

void menu_draw_banner(void) {
    /* Legacy helper — kept for compatibility; no-op in Win95 mode */
}

void menu_run(void) {
    /* ── 1. Startup splash ──────────────────────────────────────────────── */
    ui95_startup_splash();

    /* ── 2. Login loop (up to 3 attempts) ──────────────────────────────── */
    char username[USER_NAME_LEN];
    char password[USER_PASS_LEN];
    int  user_idx = -1;
    int  attempts = 0;

    while (user_idx < 0 && attempts < 3) {
        if (attempts > 0) {
            ui95_message_box("Login Failed",
                             "Incorrect username or password.", 1);
        }
        username[0] = password[0] = '\0';
        ui95_login_screen(username, password, USER_NAME_LEN);
        user_idx = user_authenticate(username, password);
        attempts++;
    }

    if (user_idx < 0) {
        ui95_message_box("AI Aura OS",
                         "Too many failed attempts. System locked.", 1);
        kernel_panic("Authentication lockout.");
        /* unreachable */
    }

    /* ── 3. Take a post-login snapshot ─────────────────────────────────── */
    mirror_capture(1, "login-snapshot", MIRROR_FLAG_ALL);

    /* ── 4. Desktop + interactive Start-menu loop ───────────────────────── */
    desktop_loop(username);

    /* Never reached — desktop_loop() is perpetual */
}

