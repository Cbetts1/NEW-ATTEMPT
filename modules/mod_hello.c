/* =============================================================================
 * AI Aura OS — Example Module: hello
 * Demonstrates the module/plugin API. Registers a plugin that prints a
 * greeting on every scheduler tick (every 100 kernel ticks).
 * =============================================================================*/

/* Access the kernel plugin API from the modules directory */
#include "../kernel/plugin.h"
#include "../kernel/vga.h"

static aura_status_t hello_init(void) {
    vga_println("[hello] Module initialized.");
    return AURA_OK;
}

static aura_status_t hello_tick(void) {
    /* Print once every 256 calls to avoid flooding the screen */
    static uint32_t count = 0;
    count++;
    if ((count & 0xFF) == 0) {
        vga_print("[hello] tick #");
        vga_print_dec(count);
        vga_putchar('\n');
    }
    return AURA_OK;
}

static aura_status_t hello_shutdown(void) {
    vga_println("[hello] Module shutting down.");
    return AURA_OK;
}

/* Called by the module loader */
int mod_hello_load(void) {
    plugin_desc_t desc = {
        .name     = "hello",
        .version  = "1.0",
        .type     = PLUGIN_TYPE_SERVICE,
        .init     = hello_init,
        .tick     = hello_tick,
        .shutdown = hello_shutdown,
        .priv     = NULL,
    };
    aura_status_t r = plugin_register(&desc);
    if (r == AURA_OK) plugin_activate("hello");
    return (r == AURA_OK) ? 0 : -1;
}
