/* =============================================================================
 * AI Aura OS — Internal Event Bus
 * File: kernel/event_bus.c
 *
 * Lightweight publish/subscribe message bus.  Events are queued in a ring
 * buffer and dispatched synchronously from the heartbeat loop.
 * =========================================================================== */
#include "include/event_bus.h"
#include "include/vga.h"

/* ── Handler registry ────────────────────────────────────────────────────── */
typedef struct {
    event_id_t      id;
    event_handler_t handler;
    uint8_t         active;
} handler_slot_t;

static handler_slot_t handlers[EVENT_MAX_HANDLERS * 8];  /* 8 IDs × 16 */
static int            handler_count = 0;

/* ── Event queue ─────────────────────────────────────────────────────────── */
static event_t queue[EVENT_QUEUE_SIZE];
static int     q_head = 0;  /* next write position */
static int     q_tail = 0;  /* next read  position */
static int     q_size = 0;

/* ── Public API ──────────────────────────────────────────────────────────── */

void event_bus_init(void)
{
    handler_count = 0;
    q_head = q_tail = q_size = 0;
    vga_puts("[EVT] Event bus initialised.\n");
}

int event_subscribe(event_id_t id, event_handler_t handler)
{
    if (handler_count >= (int)(sizeof(handlers) / sizeof(handlers[0]))) {
        vga_puts("[EVT] subscribe: handler table full\n");
        return -1;
    }
    handlers[handler_count].id      = id;
    handlers[handler_count].handler = handler;
    handlers[handler_count].active  = 1;
    handler_count++;
    return 0;
}

void event_unsubscribe(event_id_t id, event_handler_t handler)
{
    for (int i = 0; i < handler_count; i++) {
        if (handlers[i].id == id && handlers[i].handler == handler)
            handlers[i].active = 0;
    }
}

void event_publish(event_id_t id, uint32_t source, uint32_t data, const char *msg)
{
    if (q_size >= EVENT_QUEUE_SIZE) {
        vga_puts("[EVT] queue full — event dropped\n");
        return;
    }
    queue[q_head].id     = id;
    queue[q_head].source = source;
    queue[q_head].data   = data;
    queue[q_head].msg    = msg;
    q_head = (q_head + 1) % EVENT_QUEUE_SIZE;
    q_size++;
}

void event_dispatch(void)
{
    while (q_size > 0) {
        event_t *ev = &queue[q_tail];
        q_tail = (q_tail + 1) % EVENT_QUEUE_SIZE;
        q_size--;

        for (int i = 0; i < handler_count; i++) {
            if (handlers[i].active && handlers[i].id == ev->id)
                handlers[i].handler(ev);
        }
    }
}
