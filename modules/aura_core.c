/* =============================================================================
 * AI Aura OS — Built-in Core Module
 * Runs internal health checks on every tick and publishes periodic heartbeat
 * diagnostics on the event bus.
 * =============================================================================*/

#include "../kernel/plugin.h"
#include "../kernel/vga.h"
#include "../kernel/eventbus.h"

static uint32_t core_tick_count = 0;

static aura_status_t core_init(void) {
    vga_println("[CORE] Aura core module online.");
    return AURA_OK;
}

static aura_status_t core_tick(void) {
    core_tick_count++;
    /* Every 1000 ticks publish a heartbeat diagnostic on the kernel tick topic */
    if ((core_tick_count % 1000) == 0) {
        eventbus_publish(TOPIC_KERNEL_TICK, core_tick_count);
    }
    return AURA_OK;
}

static aura_status_t core_shutdown(void) {
    vga_println("[CORE] Aura core module shutting down.");
    return AURA_OK;
}

/* Called by the module loader */
int mod_aura_core_load(void) {
    plugin_desc_t desc = {
        .name     = "aura_core",
        .version  = "1.0",
        .type     = PLUGIN_TYPE_CORE,
        .init     = core_init,
        .tick     = core_tick,
        .shutdown = core_shutdown,
        .priv     = (void *)0,
    };
    aura_status_t r = plugin_register(&desc);
    if (r == AURA_OK) plugin_activate("aura_core");
    return (r == AURA_OK) ? 0 : -1;
}
