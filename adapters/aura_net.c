/* =============================================================================
 * AI Aura OS — Virtual Network Adapter (stub)
 * File: adapters/aura_net.c
 *
 * Placeholder network adapter.  Provides the adapter lifecycle hooks so the
 * plugin manager can load it cleanly.  Real network I/O is a future extension.
 * =========================================================================== */
#include "../kernel/include/plugin.h"
#include "../kernel/include/vga.h"

static int net_init(void)
{
    vga_puts("[NET] Virtual network adapter online (stub).\n");
    return 0;
}

static int net_tick(void)
{
    return 0;
}

static void net_shutdown(void)
{
    vga_puts("[NET] Virtual network adapter shutting down.\n");
}

plugin_descriptor_t aura_net_adapter = {
    .name     = "aura.net",
    .version  = 0x0100,
    .type     = PLUGIN_TYPE_ADAPTER,
    .init     = net_init,
    .tick     = net_tick,
    .shutdown = net_shutdown,
};
