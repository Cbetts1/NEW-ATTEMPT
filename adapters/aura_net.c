/* =============================================================================
 * AI Aura OS — Virtual Network Adapter (stub)
 * Placeholder network adapter.  Provides the adapter lifecycle hooks so the
 * plugin manager can load it cleanly.  Real network I/O is a future extension.
 * =============================================================================*/

#include "../kernel/plugin.h"
#include "../kernel/vga.h"

static aura_status_t net_init(void) {
    vga_println("[NET] Virtual network adapter online (stub).");
    return AURA_OK;
}

static aura_status_t net_tick(void) {
    return AURA_OK;
}

static aura_status_t net_shutdown(void) {
    vga_println("[NET] Virtual network adapter shutting down.");
    return AURA_OK;
}

/* Called to register the network adapter with the plugin manager */
void adapter_net_register(void) {
    plugin_desc_t desc = {
        .name     = "net",
        .version  = "1.0",
        .type     = PLUGIN_TYPE_ADAPTER,
        .init     = net_init,
        .tick     = net_tick,
        .shutdown = net_shutdown,
        .priv     = (void *)0,
    };
    aura_status_t r = plugin_register(&desc);
    if (r == AURA_OK) plugin_activate("net");
}
