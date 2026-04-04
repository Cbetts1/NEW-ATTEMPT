/* =============================================================================
 * AI Aura OS — Plugin / Adapter Manager header
 * File: kernel/include/plugin.h
 * =========================================================================== */
#ifndef AIOS_PLUGIN_H
#define AIOS_PLUGIN_H

#include <stdint.h>

#define PLUGIN_MAX          32
#define PLUGIN_NAME_LEN     32

/* Plugin lifecycle state */
typedef enum {
    PLUGIN_STATE_UNLOADED = 0,
    PLUGIN_STATE_LOADED,
    PLUGIN_STATE_ACTIVE,
    PLUGIN_STATE_ERROR
} plugin_state_t;

/* Plugin type tag */
typedef enum {
    PLUGIN_TYPE_MODULE  = 0,
    PLUGIN_TYPE_ADAPTER = 1,
    PLUGIN_TYPE_DRIVER  = 2
} plugin_type_t;

/* Plugin descriptor — every plugin must provide one of these */
typedef struct plugin_descriptor {
    char           name[PLUGIN_NAME_LEN];
    uint32_t       version;          /* BCD: 0x0100 = v1.0 */
    plugin_type_t  type;
    int  (*init)(void);              /* called on load; returns 0 on success */
    int  (*tick)(void);              /* called every heartbeat cycle        */
    void (*shutdown)(void);          /* called on unload                    */
} plugin_descriptor_t;

/* Plugin registry entry */
typedef struct plugin_entry {
    plugin_descriptor_t *desc;
    plugin_state_t       state;
    uint32_t             id;
} plugin_entry_t;

/* Public API */
void  plugin_manager_init(void);
int   plugin_register(plugin_descriptor_t *desc);
int   plugin_load(const char *name);
int   plugin_unload(const char *name);
void  plugin_tick_all(void);
const plugin_entry_t *plugin_find(const char *name);
void  plugin_list(void);   /* print all registered plugins via VGA */

#endif /* AIOS_PLUGIN_H */
