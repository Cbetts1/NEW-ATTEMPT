# AI Aura OS — Plugin API Reference
# File: docs/plugin_api.md

## Overview

All kernel capabilities are delivered through plugins.  A plugin is any C
translation unit that fills in a `plugin_desc_t` struct and registers it via
`plugin_register()` during kernel startup, then activates it with
`plugin_activate()`.

## `plugin_desc_t` Fields

```c
/* kernel/plugin.h */
typedef struct {
    char           name[PLUGIN_NAME_LEN];       /* unique name, e.g. "my.mod" (max 24 chars) */
    char           version[PLUGIN_VERSION_LEN]; /* version string, e.g. "1.0"  (max 8 chars) */
    plugin_type_t  type;                        /* CORE, DRIVER, SERVICE, or ADAPTER          */
    aura_status_t  (*init)(void);               /* called once on activate; AURA_OK = success */
    aura_status_t  (*tick)(void);               /* called every N scheduler ticks             */
    aura_status_t  (*shutdown)(void);           /* called on deactivate/unregister            */
    void          *priv;                        /* optional plugin-private data pointer       */
} plugin_desc_t;
```

`aura_status_t` return values (defined in `kernel/kernel.h`):

| Value          | Meaning                          |
|----------------|----------------------------------|
| `AURA_OK`      | Success                          |
| `AURA_ERR`     | Generic error                    |
| `AURA_NOMEM`   | Out of memory                    |
| `AURA_NOSLOT`  | Registry / table is full         |
| `AURA_NOTFOUND`| Name not found                   |
| `AURA_BUSY`    | Resource busy (e.g. queue full)  |

## Plugin Types

| Type                   | Purpose                                             |
|------------------------|-----------------------------------------------------|
| `PLUGIN_TYPE_CORE`     | Essential OS subsystems (schedulers, managers)      |
| `PLUGIN_TYPE_DRIVER`   | Low-level hardware abstraction                      |
| `PLUGIN_TYPE_SERVICE`  | Background OS services (e.g. mod_hello)             |
| `PLUGIN_TYPE_ADAPTER`  | Hardware/virtual device adapters (serial, net, ATA) |

## Lifecycle

```
plugin_register(&desc)
        │  slot allocated, state → PLUGIN_STATE_LOADED
        ▼
plugin_activate("name")
        │  calls desc.init(); if AURA_OK → state → PLUGIN_STATE_ACTIVE
        ▼
  PLUGIN_STATE_ACTIVE  ──┐
        │                 │  desc.tick() called by plugin_tick_all()
        │                 │  (scheduler task "plugin_tick", period 5)
        │  plugin_deactivate("name")
        │  calls desc.shutdown() → state → PLUGIN_STATE_LOADED
        ▼
  PLUGIN_STATE_LOADED
        │
  plugin_unregister("name")
        │  if ACTIVE, calls desc.shutdown() first
        ▼
  PLUGIN_STATE_UNLOADED  (slot freed)
```

On `init()` or `tick()` error the state transitions to `PLUGIN_STATE_ERROR`
and an error counter is incremented.  The manager continues ticking all other
active plugins.

## Minimal Plugin Example

```c
/* modules/my_module.c */
#include "../kernel/plugin.h"
#include "../kernel/vga.h"

static aura_status_t my_init(void) {
    vga_println("[MY] init");
    return AURA_OK;
}

static aura_status_t my_tick(void) {
    return AURA_OK;
}

static aura_status_t my_shutdown(void) {
    vga_println("[MY] shutdown");
    return AURA_OK;
}

int mod_my_load(void) {
    plugin_desc_t desc = {
        .name     = "my.module",
        .version  = "1.0",
        .type     = PLUGIN_TYPE_SERVICE,
        .init     = my_init,
        .tick     = my_tick,
        .shutdown = my_shutdown,
        .priv     = NULL,
    };
    aura_status_t r = plugin_register(&desc);
    if (r == AURA_OK) plugin_activate("my.module");
    return (r == AURA_OK) ? 0 : -1;
}
```

Register the module in `modules/loader.c`:

```c
extern int mod_my_load(void);

static module_entry_t module_registry[] = {
    { "hello",     mod_hello_load,     0 },
    { "aura_core", mod_aura_core_load, 0 },
    { "my.module", mod_my_load,        0 },  /* ← add here */
    { NULL, NULL, 0 },
};
```

Add the source to `Makefile` `MODULE_SRCS`:

```makefile
MODULE_SRCS := \
    modules/loader.c    \
    modules/mod_hello.c \
    modules/aura_core.c \
    modules/my_module.c
```

## Event Bus Integration

Plugins can publish and subscribe to events using `kernel/eventbus.h`:

```c
#include "../kernel/eventbus.h"

static void on_kernel_tick(const aura_event_t *ev) {
    /* ev->data   = tick count published by aura_core every 1000 ticks */
    /* ev->topic  = TOPIC_KERNEL_TICK                                   */
    /* ev->timestamp = g_tick_count at publish time                     */
}

static aura_status_t my_init(void) {
    eventbus_subscribe(TOPIC_KERNEL_TICK, on_kernel_tick);
    return AURA_OK;
}

static aura_status_t my_shutdown(void) {
    eventbus_unsubscribe(TOPIC_KERNEL_TICK, on_kernel_tick);
    return AURA_OK;
}
```

Well-known topic IDs (defined in `kernel/eventbus.h`):

| Topic ID                    | Published by          | `data` value              |
|-----------------------------|-----------------------|---------------------------|
| `TOPIC_KERNEL_TICK`         | `aura_core` (×1000)   | core tick counter         |
| `TOPIC_PLUGIN_LOADED`       | plugin manager        | slot index                |
| `TOPIC_PLUGIN_UNLOADED`     | plugin manager        | 0                         |
| `TOPIC_MIRROR_SYNC`         | mirror engine (×256)  | sync counter              |
| `TOPIC_SCHEDULER_SWITCH`    | scheduler             | scheduler tick            |
| `TOPIC_ADAPTER_CONNECTED`   | adapter layer         | adapter ID                |
| `TOPIC_ADAPTER_DISCONNECTED`| adapter layer         | adapter ID                |
| `TOPIC_USER_INPUT`          | keyboard / serial     | ASCII char (low byte)     |
| `TOPIC_PANIC`               | kernel_panic()        | 0                         |
