/* =============================================================================
 * AI Aura OS — Event Bus header
 * File: kernel/include/event_bus.h
 * =========================================================================== */
#ifndef AIOS_EVENT_BUS_H
#define AIOS_EVENT_BUS_H

#include <stdint.h>

/* Maximum simultaneously registered handlers per event type */
#define EVENT_MAX_HANDLERS  16
/* Maximum queued events */
#define EVENT_QUEUE_SIZE    64

/* Predefined system event IDs */
typedef enum {
    EVENT_KERNEL_READY   = 0x01,
    EVENT_HEARTBEAT      = 0x02,
    EVENT_PLUGIN_LOADED  = 0x10,
    EVENT_PLUGIN_UNLOAD  = 0x11,
    EVENT_ADAPTER_READY  = 0x20,
    EVENT_MIRROR_SYNC    = 0x30,
    EVENT_MENU_SELECT    = 0x40,
    EVENT_SHUTDOWN       = 0xFF,
    EVENT_USER_BASE      = 0x100   /* user events start here */
} event_id_t;

/* Event payload envelope */
typedef struct {
    event_id_t  id;
    uint32_t    source;     /* originator subsystem ID */
    uint32_t    data;       /* generic payload */
    const char *msg;        /* optional string payload (may be NULL) */
} event_t;

/* Handler callback signature */
typedef void (*event_handler_t)(const event_t *event);

/* Public API */
void event_bus_init(void);
int  event_subscribe(event_id_t id, event_handler_t handler);
void event_unsubscribe(event_id_t id, event_handler_t handler);
void event_publish(event_id_t id, uint32_t source, uint32_t data, const char *msg);
void event_dispatch(void);   /* drain the queue — call from heartbeat */

#endif /* AIOS_EVENT_BUS_H */
