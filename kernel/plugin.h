#ifndef PLUGIN_H
#define PLUGIN_H

/* =============================================================================
 * AI Aura OS — Plugin / Adapter Manager Interface
 * Plugins register themselves with a descriptor; the manager tracks lifecycle.
 * =============================================================================*/

#include <stdint.h>
#include "kernel.h"

#define PLUGIN_MAX          16
#define PLUGIN_NAME_LEN     24
#define PLUGIN_VERSION_LEN  8

/* Plugin lifecycle stages */
typedef enum {
    PLUGIN_STATE_UNLOADED = 0,
    PLUGIN_STATE_LOADED   = 1,
    PLUGIN_STATE_ACTIVE   = 2,
    PLUGIN_STATE_ERROR    = 3,
} plugin_state_t;

/* Plugin type */
typedef enum {
    PLUGIN_TYPE_CORE    = 0,
    PLUGIN_TYPE_DRIVER  = 1,
    PLUGIN_TYPE_SERVICE = 2,
    PLUGIN_TYPE_ADAPTER = 3,
} plugin_type_t;

/* Plugin descriptor — filled in by each plugin at registration */
typedef struct {
    char           name[PLUGIN_NAME_LEN];
    char           version[PLUGIN_VERSION_LEN];
    plugin_type_t  type;
    aura_status_t  (*init)(void);
    aura_status_t  (*tick)(void);       /* Called each scheduler tick */
    aura_status_t  (*shutdown)(void);
    void          *priv;                /* Plugin private data pointer */
} plugin_desc_t;

/* Plugin slot (managed by plugin manager) */
typedef struct {
    plugin_desc_t  desc;
    plugin_state_t state;
    uint32_t       load_tick;
    uint32_t       tick_count;
    uint32_t       error_count;
} plugin_slot_t;

void          plugin_manager_init(void);
aura_status_t plugin_register(const plugin_desc_t *desc);
aura_status_t plugin_unregister(const char *name);
aura_status_t plugin_activate(const char *name);
aura_status_t plugin_deactivate(const char *name);
void          plugin_tick_all(void);
plugin_slot_t *plugin_find(const char *name);
void          plugin_list(void);        /* Print all plugins to VGA */
int           plugin_count(void);

#endif /* PLUGIN_H */
