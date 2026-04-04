/* =============================================================================
 * AI Aura OS — Cooperative Task Scheduler Implementation
 * =============================================================================*/

#include "scheduler.h"
#include "vga.h"
#include "eventbus.h"
#include "kstring.h"

static task_t   task_table[SCHED_MAX_TASKS];
static int      task_count_val = 0;
static uint32_t sched_tick     = 0;

/* -------------------------------------------------------------------------- */

void scheduler_init(void) {
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        task_table[i].state = TASK_STATE_EMPTY;
        task_table[i].tick_count = 0;
    }
    task_count_val = 0;
    sched_tick     = 0;
}

aura_status_t scheduler_add_task(const char *name, task_fn_t fn, uint32_t period) {
    if (!name || !fn)              return AURA_ERR;
    if (task_count_val >= SCHED_MAX_TASKS) return AURA_NOSLOT;

    int slot = -1;
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (task_table[i].state == TASK_STATE_EMPTY) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return AURA_NOSLOT;

    strncpy_k(task_table[slot].name, name, TASK_NAME_LEN);
    task_table[slot].fn        = fn;
    task_table[slot].state     = TASK_STATE_READY;
    task_table[slot].period    = period;
    task_table[slot].last_run  = 0;
    task_table[slot].tick_count= 0;
    task_count_val++;
    return AURA_OK;
}

aura_status_t scheduler_remove_task(const char *name) {
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (task_table[i].state != TASK_STATE_EMPTY &&
            strncmp_k(task_table[i].name, name, TASK_NAME_LEN) == 0) {
            task_table[i].state = TASK_STATE_ZOMBIE;
            task_count_val--;
            return AURA_OK;
        }
    }
    return AURA_NOTFOUND;
}

void scheduler_tick(void) {
    sched_tick++;

    /* Clean up zombies */
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (task_table[i].state == TASK_STATE_ZOMBIE) {
            task_table[i].state = TASK_STATE_EMPTY;
        }
    }

    /* Run ready tasks whose period has elapsed */
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (task_table[i].state != TASK_STATE_READY) continue;
        uint32_t period = task_table[i].period;
        if (period == 0 || (sched_tick - task_table[i].last_run) >= period) {
            task_table[i].state    = TASK_STATE_RUNNING;
            task_table[i].last_run = sched_tick;
            task_table[i].fn();
            task_table[i].tick_count++;
            task_table[i].state = TASK_STATE_READY;
        }
    }

    eventbus_publish(TOPIC_SCHEDULER_SWITCH, sched_tick);
}

int scheduler_task_count(void) {
    return task_count_val;
}

void scheduler_list(void) {
    vga_println("--- Scheduler Tasks ---");
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        if (task_table[i].state == TASK_STATE_EMPTY) continue;
        vga_print("  [");
        vga_print(task_table[i].name);
        vga_print("] state=");
        switch (task_table[i].state) {
            case TASK_STATE_READY:   vga_print("READY");   break;
            case TASK_STATE_RUNNING: vga_print("RUNNING"); break;
            case TASK_STATE_BLOCKED: vga_print("BLOCKED"); break;
            default:                 vga_print("?");       break;
        }
        vga_print(" period="); vga_print_dec(task_table[i].period);
        vga_print(" runs=");   vga_print_dec(task_table[i].tick_count);
        vga_putchar('\n');
    }
}
