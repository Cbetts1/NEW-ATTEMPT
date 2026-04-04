/* =============================================================================
 * AI Aura OS — Plugin / Adapter Manager Implementation
 * =============================================================================*/

#include "plugin.h"
#include "vga.h"
#include "eventbus.h"
#include "memory.h"

static plugin_slot_t plugin_table[PLUGIN_MAX];
static int           plugin_count_val = 0;

/* -------------------------------------------------------------------------- */

static int strncmp_k(const char *a, const char *b, int n) {
    while (n-- > 0) {
        if (*a != *b) return (int)(unsigned char)*a - (int)(unsigned char)*b;
        if (*a == '\0') return 0;
        a++; b++;
    }
    return 0;
}

static void strncpy_k(char *dst, const char *src, int n) {
    int i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

/* -------------------------------------------------------------------------- */

void plugin_manager_init(void) {
    for (int i = 0; i < PLUGIN_MAX; i++) {
        plugin_table[i].state       = PLUGIN_STATE_UNLOADED;
        plugin_table[i].tick_count  = 0;
        plugin_table[i].error_count = 0;
    }
    plugin_count_val = 0;
}

aura_status_t plugin_register(const plugin_desc_t *desc) {
    if (!desc) return AURA_ERR;
    if (plugin_count_val >= PLUGIN_MAX) return AURA_NOSLOT;

    /* Check for duplicate name */
    if (plugin_find(desc->name)) return AURA_ERR;

    int slot = -1;
    for (int i = 0; i < PLUGIN_MAX; i++) {
        if (plugin_table[i].state == PLUGIN_STATE_UNLOADED) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return AURA_NOSLOT;

    plugin_table[slot].desc       = *desc;
    plugin_table[slot].state      = PLUGIN_STATE_LOADED;
    plugin_table[slot].tick_count = 0;
    plugin_table[slot].error_count= 0;

    extern volatile uint32_t g_tick_count;
    plugin_table[slot].load_tick  = g_tick_count;
    plugin_count_val++;

    eventbus_publish(TOPIC_PLUGIN_LOADED, (uint32_t)slot);
    return AURA_OK;
}

aura_status_t plugin_unregister(const char *name) {
    plugin_slot_t *s = plugin_find(name);
    if (!s) return AURA_NOTFOUND;
    if (s->state == PLUGIN_STATE_ACTIVE && s->desc.shutdown) {
        s->desc.shutdown();
    }
    eventbus_publish(TOPIC_PLUGIN_UNLOADED, 0);
    s->state = PLUGIN_STATE_UNLOADED;
    plugin_count_val--;
    return AURA_OK;
}

aura_status_t plugin_activate(const char *name) {
    plugin_slot_t *s = plugin_find(name);
    if (!s) return AURA_NOTFOUND;
    if (s->state != PLUGIN_STATE_LOADED) return AURA_ERR;
    if (s->desc.init) {
        aura_status_t r = s->desc.init();
        if (r != AURA_OK) {
            s->state = PLUGIN_STATE_ERROR;
            s->error_count++;
            return r;
        }
    }
    s->state = PLUGIN_STATE_ACTIVE;
    return AURA_OK;
}

aura_status_t plugin_deactivate(const char *name) {
    plugin_slot_t *s = plugin_find(name);
    if (!s) return AURA_NOTFOUND;
    if (s->state != PLUGIN_STATE_ACTIVE) return AURA_ERR;
    if (s->desc.shutdown) s->desc.shutdown();
    s->state = PLUGIN_STATE_LOADED;
    return AURA_OK;
}

void plugin_tick_all(void) {
    for (int i = 0; i < PLUGIN_MAX; i++) {
        if (plugin_table[i].state == PLUGIN_STATE_ACTIVE &&
            plugin_table[i].desc.tick) {
            aura_status_t r = plugin_table[i].desc.tick();
            plugin_table[i].tick_count++;
            if (r != AURA_OK) {
                plugin_table[i].error_count++;
            }
        }
    }
}

plugin_slot_t *plugin_find(const char *name) {
    for (int i = 0; i < PLUGIN_MAX; i++) {
        if (plugin_table[i].state != PLUGIN_STATE_UNLOADED &&
            strncmp_k(plugin_table[i].desc.name, name, PLUGIN_NAME_LEN) == 0) {
            return &plugin_table[i];
        }
    }
    return NULL;
}

void plugin_list(void) {
    vga_println("--- Plugins ---");
    for (int i = 0; i < PLUGIN_MAX; i++) {
        if (plugin_table[i].state == PLUGIN_STATE_UNLOADED) continue;
        vga_print("  [");
        vga_print(plugin_table[i].desc.name);
        vga_print("] v");
        vga_print(plugin_table[i].desc.version);
        vga_print("  state=");
        switch (plugin_table[i].state) {
            case PLUGIN_STATE_LOADED:  vga_print("LOADED");  break;
            case PLUGIN_STATE_ACTIVE:  vga_print("ACTIVE");  break;
            case PLUGIN_STATE_ERROR:   vga_print("ERROR");   break;
            default:                   vga_print("?");       break;
        }
        vga_print("  ticks=");
        vga_print_dec(plugin_table[i].tick_count);
        vga_putchar('\n');
    }
}

int plugin_count(void) {
    return plugin_count_val;
}
