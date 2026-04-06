#ifndef SCHEDULER_H
#define SCHEDULER_H

/* =============================================================================
 * AI Aura OS — Cooperative Task Scheduler Interface
 * Simple round-robin cooperative scheduler for kernel tasks.
 * =============================================================================*/

#include <stdint.h>
#include "kernel.h"

#define SCHED_MAX_TASKS   16
#define SCHED_STACK_SIZE  4096
#define TASK_NAME_LEN     16

typedef enum {
    TASK_STATE_EMPTY   = 0,
    TASK_STATE_READY   = 1,
    TASK_STATE_RUNNING = 2,
    TASK_STATE_BLOCKED = 3,
    TASK_STATE_ZOMBIE  = 4,
} task_state_t;

typedef void (*task_fn_t)(void);

typedef struct {
    char         name[TASK_NAME_LEN];
    task_fn_t    fn;
    task_state_t state;
    uint32_t     tick_count;
    uint32_t     period;        /* Run every N ticks (0 = every tick) */
    uint32_t     last_run;
} task_t;

void          scheduler_init(void);
aura_status_t scheduler_add_task(const char *name, task_fn_t fn, uint32_t period);
aura_status_t scheduler_remove_task(const char *name);
void          scheduler_tick(void);
int           scheduler_task_count(void);
void          scheduler_list(void);

#endif /* SCHEDULER_H */
