/* =============================================================================
 * AI Aura OS — Module Loader Implementation
 * The loader maintains a static registry of modules and triggers their
 * load() function which registers a plugin_desc_t with the plugin manager.
 * =============================================================================*/

#include "loader.h"
#include <stddef.h>
#include "../kernel/kstring.h"

/* Forward declarations for built-in modules */
extern int mod_hello_load(void);
extern int mod_aura_core_load(void);

/* Static module registry */
static module_entry_t module_registry[] = {
    { "hello",     mod_hello_load,     0 },
    { "aura_core", mod_aura_core_load, 0 },
    { NULL,        NULL,               0 },
};

/* -------------------------------------------------------------------------- */

void loader_init(void) {
    for (int i = 0; module_registry[i].name; i++) {
        module_registry[i].loaded = 0;
    }
}

int loader_load(const char *name) {
    for (int i = 0; module_registry[i].name; i++) {
        if (strncmp_k(module_registry[i].name, name, MODULE_NAME_LEN) == 0) {
            if (module_registry[i].loaded) return 0; /* Already loaded */
            int r = module_registry[i].load();
            if (r == 0) module_registry[i].loaded = 1;
            return r;
        }
    }
    return -1; /* Not found */
}

int loader_unload(const char *name) {
    for (int i = 0; module_registry[i].name; i++) {
        if (strncmp_k(module_registry[i].name, name, MODULE_NAME_LEN) == 0) {
            module_registry[i].loaded = 0;
            return 0;
        }
    }
    return -1;
}

void loader_load_all(void) {
    for (int i = 0; module_registry[i].name; i++) {
        if (!module_registry[i].loaded) {
            loader_load(module_registry[i].name);
        }
    }
}

void loader_list(void) {
    /* Listing is performed by the caller using the VGA driver to avoid a
     * circular dependency between the module loader and the display layer.
     * Iterate module_registry[] directly and call vga_print() from context
     * where the VGA driver is available. */
    (void)module_registry;
}
