#ifndef LOADER_H
#define LOADER_H

/* =============================================================================
 * AI Aura OS — Module Loader Interface
 * Loads statically-linked modules from the module registry and activates them
 * via the plugin manager.
 * =============================================================================*/

#include <stdint.h>

#define LOADER_MAX_MODULES  8
#define MODULE_NAME_LEN     24

typedef struct {
    const char *name;
    int (*load)(void);      /* Returns 0 on success */
    int loaded;
} module_entry_t;

void loader_init(void);
int  loader_load(const char *name);
int  loader_unload(const char *name);
void loader_load_all(void);
void loader_list(void);
void loader_list_vga(void);         /* Print all modules to VGA */

#endif /* LOADER_H */
