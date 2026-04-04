#ifndef ADAPTER_H
#define ADAPTER_H

/* =============================================================================
 * AI Aura OS — Adapter Interface
 * Adapters bridge the kernel to virtual or real hardware devices.
 * Each adapter implements the adapter_ops_t interface and registers itself
 * with the plugin manager as PLUGIN_TYPE_ADAPTER.
 * =============================================================================*/

#include <stdint.h>
#include "../kernel/kernel.h"

#define ADAPTER_NAME_LEN  24

typedef struct adapter_ops {
    aura_status_t (*open)(void);
    aura_status_t (*close)(void);
    int           (*read)(uint8_t *buf, uint32_t len);
    int           (*write)(const uint8_t *buf, uint32_t len);
    aura_status_t (*ioctl)(uint32_t cmd, void *arg);
} adapter_ops_t;

typedef struct {
    char          name[ADAPTER_NAME_LEN];
    uint32_t      flags;
    adapter_ops_t ops;
    uint8_t       open;
} adapter_t;

/* Register an adapter as a plugin */
aura_status_t adapter_register(adapter_t *adp);

/* Register the built-in COM1 serial adapter */
void adapter_serial_register(void);

#endif /* ADAPTER_H */
