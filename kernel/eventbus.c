/* =============================================================================
 * AI Aura OS — Event Bus Implementation
 * =============================================================================*/

#include "eventbus.h"
#include "memory.h"

/* Handler table: per-topic list of callbacks */
static event_handler_t handlers[EVENTBUS_MAX_TOPICS][EVENTBUS_MAX_HANDLERS];

/* Circular queue for pending events */
static aura_event_t  queue[EVENTBUS_QUEUE_DEPTH];
static uint8_t       q_head;
static uint8_t       q_tail;
static uint32_t      q_count;

/* -------------------------------------------------------------------------- */

void eventbus_init(void) {
    for (int t = 0; t < EVENTBUS_MAX_TOPICS; t++) {
        for (int h = 0; h < EVENTBUS_MAX_HANDLERS; h++) {
            handlers[t][h] = NULL;
        }
    }
    q_head  = 0;
    q_tail  = 0;
    q_count = 0;
}

aura_status_t eventbus_subscribe(uint8_t topic, event_handler_t handler) {
    if (topic >= EVENTBUS_MAX_TOPICS || !handler) return AURA_ERR;
    for (int h = 0; h < EVENTBUS_MAX_HANDLERS; h++) {
        if (handlers[topic][h] == NULL) {
            handlers[topic][h] = handler;
            return AURA_OK;
        }
    }
    return AURA_NOSLOT;
}

aura_status_t eventbus_unsubscribe(uint8_t topic, event_handler_t handler) {
    if (topic >= EVENTBUS_MAX_TOPICS || !handler) return AURA_ERR;
    for (int h = 0; h < EVENTBUS_MAX_HANDLERS; h++) {
        if (handlers[topic][h] == handler) {
            handlers[topic][h] = NULL;
            return AURA_OK;
        }
    }
    return AURA_NOTFOUND;
}

aura_status_t eventbus_publish(uint8_t topic, uint32_t data) {
    if (topic >= EVENTBUS_MAX_TOPICS) return AURA_ERR;
    if (q_count >= EVENTBUS_QUEUE_DEPTH) return AURA_BUSY;

    extern volatile uint32_t g_tick_count;
    queue[q_tail].topic     = topic;
    queue[q_tail].data      = data;
    queue[q_tail].timestamp = g_tick_count;
    q_tail = (uint8_t)((q_tail + 1) % EVENTBUS_QUEUE_DEPTH);
    q_count++;
    return AURA_OK;
}

void eventbus_process(void) {
    while (q_count > 0) {
        aura_event_t evt = queue[q_head];
        q_head  = (uint8_t)((q_head + 1) % EVENTBUS_QUEUE_DEPTH);
        q_count--;

        if (evt.topic < EVENTBUS_MAX_TOPICS) {
            for (int h = 0; h < EVENTBUS_MAX_HANDLERS; h++) {
                if (handlers[evt.topic][h]) {
                    handlers[evt.topic][h](&evt);
                }
            }
        }
    }
}

uint32_t eventbus_pending(void) {
    return q_count;
}
