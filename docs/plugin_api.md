# AI Aura OS — Plugin API Reference
# File: docs/plugin_api.md

## Overview

All kernel capabilities are delivered through plugins.  A plugin is any C
translation unit that exports a `plugin_descriptor_t` struct and is registered
via `plugin_register()` during kernel startup.

## `plugin_descriptor_t` Fields

```c
typedef struct plugin_descriptor {
    char           name[32];   // unique dot-separated name, e.g. "aura.mymod"
    uint32_t       version;    // BCD version: 0x0100 = v1.0
    plugin_type_t  type;       // MODULE, ADAPTER, or DRIVER
    int  (*init)(void);        // called once on plugin_load(); 0 = success
    int  (*tick)(void);        // called every heartbeat cycle; 0 = continue
    void (*shutdown)(void);    // called on plugin_unload()
} plugin_descriptor_t;
```

## Lifecycle

```
plugin_register(desc)
        │
        ▼
  PLUGIN_STATE_LOADED
        │
   plugin_load(name)
        │  calls init()
        ▼
  PLUGIN_STATE_ACTIVE  ──┐
        │                 │  tick() called every heartbeat
        │  plugin_unload  │
        ▼                 │
  PLUGIN_STATE_LOADED  ◄──┘
```

## Minimal Plugin Example

```c
// modules/my_module.c
#include "../kernel/include/plugin.h"
#include "../kernel/include/vga.h"

static int my_init(void)     { vga_puts("[MY] init\n"); return 0; }
static int my_tick(void)     { return 0; }
static void my_shutdown(void){ vga_puts("[MY] shutdown\n"); }

plugin_descriptor_t my_module_plugin = {
    .name     = "my.module",
    .version  = 0x0100,
    .type     = PLUGIN_TYPE_MODULE,
    .init     = my_init,
    .tick     = my_tick,
    .shutdown = my_shutdown,
};
```

Then in `kernel/kernel.c`:

```c
extern plugin_descriptor_t my_module_plugin;
// ...
plugin_register(&my_module_plugin);
plugin_load("my.module");
```

And in `Makefile` KERNEL_C_SRCS:

```makefile
KERNEL_C_SRCS += modules/my_module.c
```

## Event Bus Integration

Plugins can publish and subscribe to events:

```c
#include "../kernel/include/event_bus.h"

static void on_heartbeat(const event_t *ev) {
    // respond to heartbeat
}

static int my_init(void) {
    event_subscribe(EVENT_HEARTBEAT, on_heartbeat);
    return 0;
}
```

## Plugin Types

| Type                  | Purpose                                           |
|-----------------------|---------------------------------------------------|
| `PLUGIN_TYPE_MODULE`  | Core OS functionality (schedulers, managers)      |
| `PLUGIN_TYPE_ADAPTER` | Virtualised external interfaces (FS, net, device) |
| `PLUGIN_TYPE_DRIVER`  | Low-level hardware abstraction                    |
