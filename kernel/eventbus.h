#ifndef EVENTBUS_H
#define EVENTBUS_H

/* =============================================================================
 * AI Aura OS — Internal Event Bus
 * Subscribers register callbacks; publishers post events by topic ID.
 * =============================================================================*/

#include <stdint.h>
#include "kernel.h"

#define EVENTBUS_MAX_TOPICS       32
#define EVENTBUS_MAX_HANDLERS     8
#define EVENTBUS_QUEUE_DEPTH      64

/* Well-known topic IDs */
#define TOPIC_KERNEL_TICK         0
#define TOPIC_PLUGIN_LOADED       1
#define TOPIC_PLUGIN_UNLOADED     2
#define TOPIC_MIRROR_SYNC         3
#define TOPIC_SCHEDULER_SWITCH    4
#define TOPIC_ADAPTER_CONNECTED   5
#define TOPIC_ADAPTER_DISCONNECTED 6
#define TOPIC_USER_INPUT          7
#define TOPIC_PANIC               31

typedef struct {
    uint8_t  topic;
    uint32_t data;
    uint32_t timestamp;
} aura_event_t;

typedef void (*event_handler_t)(const aura_event_t *evt);

void          eventbus_init(void);
aura_status_t eventbus_subscribe(uint8_t topic, event_handler_t handler);
aura_status_t eventbus_unsubscribe(uint8_t topic, event_handler_t handler);
aura_status_t eventbus_publish(uint8_t topic, uint32_t data);
void          eventbus_process(void);   /* Drain the pending queue */
uint32_t      eventbus_pending(void);   /* Count of queued events */

#endif /* EVENTBUS_H */
