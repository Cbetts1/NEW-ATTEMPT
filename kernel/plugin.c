/* =============================================================================
 * AI Aura OS — Plugin / Adapter Manager
 * File: kernel/plugin.c
 *
 * Central registry for all loadable modules, adapters, and drivers.
 * Plugins are registered at compile-time; the manager can activate/deactivate
 * them and drives their tick() callbacks from the heartbeat loop.
 * =========================================================================== */
#include "include/plugin.h"
#include "include/vga.h"
#include "include/event_bus.h"
#include "include/memory.h"

static plugin_entry_t registry[PLUGIN_MAX];
static int            plugin_count = 0;
static uint32_t       next_id      = 1;

/* ── string helper ───────────────────────────────────────────────────────── */
static int kstrcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void plugin_manager_init(void)
{
    plugin_count = 0;
    next_id      = 1;
    kmemset(registry, 0, sizeof(registry));
    vga_puts("[PLG] Plugin manager initialised.\n");
}

int plugin_register(plugin_descriptor_t *desc)
{
    if (!desc) return -1;
    if (plugin_count >= PLUGIN_MAX) {
        vga_puts("[PLG] register: registry full\n");
        return -1;
    }
    registry[plugin_count].desc  = desc;
    registry[plugin_count].state = PLUGIN_STATE_LOADED;
    registry[plugin_count].id    = next_id++;
    plugin_count++;

    vga_printf("[PLG] Registered: %s (v%x)\n", desc->name, desc->version);
    return 0;
}

int plugin_load(const char *name)
{
    for (int i = 0; i < plugin_count; i++) {
        if (kstrcmp(registry[i].desc->name, name) == 0) {
            if (registry[i].state == PLUGIN_STATE_ACTIVE) return 0;
            int rc = 0;
            if (registry[i].desc->init)
                rc = registry[i].desc->init();
            if (rc == 0) {
                registry[i].state = PLUGIN_STATE_ACTIVE;
                event_publish(EVENT_PLUGIN_LOADED, registry[i].id, 0, name);
                vga_printf("[PLG] Loaded: %s\n", name);
            } else {
                registry[i].state = PLUGIN_STATE_ERROR;
                vga_printf("[PLG] LOAD ERROR: %s (rc=%d)\n", name, rc);
            }
            return rc;
        }
    }
    vga_printf("[PLG] load: plugin '%s' not found\n", name);
    return -1;
}

int plugin_unload(const char *name)
{
    for (int i = 0; i < plugin_count; i++) {
        if (kstrcmp(registry[i].desc->name, name) == 0) {
            if (registry[i].desc->shutdown)
                registry[i].desc->shutdown();
            event_publish(EVENT_PLUGIN_UNLOAD, registry[i].id, 0, name);
            registry[i].state = PLUGIN_STATE_LOADED;
            vga_printf("[PLG] Unloaded: %s\n", name);
            return 0;
        }
    }
    return -1;
}

void plugin_tick_all(void)
{
    for (int i = 0; i < plugin_count; i++) {
        if (registry[i].state == PLUGIN_STATE_ACTIVE &&
            registry[i].desc->tick) {
            registry[i].desc->tick();
        }
    }
}

const plugin_entry_t *plugin_find(const char *name)
{
    for (int i = 0; i < plugin_count; i++) {
        if (kstrcmp(registry[i].desc->name, name) == 0)
            return &registry[i];
    }
    return (void *)0;
}

void plugin_list(void)
{
    static const char * const state_names[] = {
        "UNLOADED", "LOADED", "ACTIVE", "ERROR"
    };
    vga_puts("[PLG] Plugin registry:\n");
    for (int i = 0; i < plugin_count; i++) {
        vga_printf("  [%u] %-20s state=%-8s type=%d ver=0x%x\n",
                   registry[i].id,
                   registry[i].desc->name,
                   state_names[registry[i].state],
                   (int)registry[i].desc->type,
                   registry[i].desc->version);
    }
}
