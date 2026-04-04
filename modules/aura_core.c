/* =============================================================================
 * AI Aura OS — Built-in Core Plugin
 * File: modules/aura_core.c
 *
 * The core system plugin.  Runs internal health checks on every tick and
 * exposes the kernel's own state through the plugin interface.
 * =========================================================================== */
#include "../kernel/include/plugin.h"
#include "../kernel/include/vga.h"
#include "../kernel/include/event_bus.h"

static uint32_t core_tick_count = 0;

static int core_init(void)
{
    vga_puts("[CORE] Aura core plugin online.\n");
    return 0;
}

static int core_tick(void)
{
    core_tick_count++;
    /* Every 1000 ticks publish a heartbeat diagnostic */
    if (core_tick_count % 1000 == 0)
        event_publish(EVENT_HEARTBEAT, 0x01, core_tick_count, "core diagnostic");
    return 0;
}

static void core_shutdown(void)
{
    vga_puts("[CORE] Aura core plugin shutting down.\n");
}

plugin_descriptor_t aura_core_plugin = {
    .name     = "aura.core",
    .version  = 0x0100,
    .type     = PLUGIN_TYPE_MODULE,
    .init     = core_init,
    .tick     = core_tick,
    .shutdown = core_shutdown,
};
