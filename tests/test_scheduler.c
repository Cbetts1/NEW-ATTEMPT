/* =============================================================================
 * AI Aura OS — Scheduler Unit Tests
 * =============================================================================*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tests/framework.h"
#include "kernel/scheduler.h"
#include "kernel/eventbus.h"

/* =========================================================================== */
/* Helper task functions                                                        */
/* =========================================================================== */

static int s_task_a_runs = 0;
static int s_task_b_runs = 0;
static int s_task_c_runs = 0;

static void task_a(void) { s_task_a_runs++; }
static void task_b(void) { s_task_b_runs++; }
static void task_c(void) { s_task_c_runs++; }

static void reset_task_counters(void) {
    s_task_a_runs = 0;
    s_task_b_runs = 0;
    s_task_c_runs = 0;
}

/* Initialize both the scheduler and the event bus (scheduler_tick publishes). */
static void sched_setup(void) {
    eventbus_init();
    scheduler_init();
    reset_task_counters();
}

/* =========================================================================== */
/* Tests                                                                        */
/* =========================================================================== */

static void test_init_count_zero(void) {
    sched_setup();
    TEST_ASSERT_EQ(scheduler_task_count(), 0);
}

static void test_add_task_returns_ok(void) {
    sched_setup();
    aura_status_t r = scheduler_add_task("alpha", task_a, 0);
    TEST_ASSERT_EQ(r, AURA_OK);
    TEST_ASSERT_EQ(scheduler_task_count(), 1);
}

static void test_add_task_null_name(void) {
    sched_setup();
    aura_status_t r = scheduler_add_task(NULL, task_a, 0);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_add_task_null_fn(void) {
    sched_setup();
    aura_status_t r = scheduler_add_task("beta", NULL, 0);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_add_task_max_slots(void) {
    sched_setup();
    /* Fill all available slots */
    char name[TASK_NAME_LEN];
    for (int i = 0; i < SCHED_MAX_TASKS; i++) {
        snprintf(name, sizeof(name), "task%d", i);
        aura_status_t r = scheduler_add_task(name, task_a, 0);
        TEST_ASSERT_EQ(r, AURA_OK);
    }
    TEST_ASSERT_EQ(scheduler_task_count(), SCHED_MAX_TASKS);
    /* One more must fail */
    aura_status_t r = scheduler_add_task("overflow", task_a, 0);
    TEST_ASSERT_EQ(r, AURA_NOSLOT);
}

static void test_remove_task_returns_ok(void) {
    sched_setup();
    scheduler_add_task("gamma", task_a, 0);
    aura_status_t r = scheduler_remove_task("gamma");
    TEST_ASSERT_EQ(r, AURA_OK);
    TEST_ASSERT_EQ(scheduler_task_count(), 0);
}

static void test_remove_task_not_found(void) {
    sched_setup();
    aura_status_t r = scheduler_remove_task("ghost");
    TEST_ASSERT_EQ(r, AURA_NOTFOUND);
}

static void test_task_runs_on_tick(void) {
    sched_setup();
    scheduler_add_task("runner", task_a, 0);
    scheduler_tick();
    TEST_ASSERT_EQ(s_task_a_runs, 1);
}

static void test_task_period_zero_runs_every_tick(void) {
    sched_setup();
    scheduler_add_task("every", task_a, 0);
    for (int i = 0; i < 5; i++)
        scheduler_tick();
    TEST_ASSERT_EQ(s_task_a_runs, 5);
}

static void test_task_period_n_respects_period(void) {
    sched_setup();
    scheduler_add_task("periodic", task_b, 3);
    /* Tick 5 times; task should run at ticks 3 and not yet at tick 6. */
    for (int i = 0; i < 5; i++)
        scheduler_tick();
    /* After 5 ticks:
     *   sched_tick becomes 5
     *   At tick 1: 1-0=1 < 3 → skip
     *   At tick 3: 3-0=3 >= 3 → run (last_run=3), task_b_runs=1
     *   At tick 4: 4-3=1 < 3 → skip
     *   At tick 5: 5-3=2 < 3 → skip
     */
    TEST_ASSERT_EQ(s_task_b_runs, 1);
}

static void test_task_period_fires_multiple_times(void) {
    sched_setup();
    scheduler_add_task("multi", task_b, 2);
    for (int i = 0; i < 6; i++)
        scheduler_tick();
    /* Fires at ticks 2, 4, 6 → 3 times */
    TEST_ASSERT_EQ(s_task_b_runs, 3);
}

static void test_zombie_cleaned_up_next_tick(void) {
    sched_setup();
    scheduler_add_task("mortal", task_c, 0);
    scheduler_tick(); /* first tick: task runs */
    TEST_ASSERT_EQ(s_task_c_runs, 1);

    scheduler_remove_task("mortal"); /* marked ZOMBIE */
    TEST_ASSERT_EQ(scheduler_task_count(), 0); /* count decremented immediately */

    scheduler_tick(); /* zombie cleaned up; task should NOT run again */
    TEST_ASSERT_EQ(s_task_c_runs, 1);
}

static void test_multiple_tasks_all_run(void) {
    sched_setup();
    scheduler_add_task("ta", task_a, 0);
    scheduler_add_task("tb", task_b, 0);
    scheduler_add_task("tc", task_c, 0);
    scheduler_tick();
    TEST_ASSERT_EQ(s_task_a_runs, 1);
    TEST_ASSERT_EQ(s_task_b_runs, 1);
    TEST_ASSERT_EQ(s_task_c_runs, 1);
}

static void test_tick_publishes_event(void) {
    sched_setup();
    scheduler_add_task("ev", task_a, 0);
    uint32_t before = eventbus_pending();
    scheduler_tick();
    /* scheduler_tick() publishes TOPIC_SCHEDULER_SWITCH */
    TEST_ASSERT(eventbus_pending() > before);
}

static void test_name_truncation(void) {
    /* Names longer than TASK_NAME_LEN-1 chars must be stored truncated, not crash */
    sched_setup();
    aura_status_t r = scheduler_add_task(
        "this_name_is_way_too_long_for_the_buffer", task_a, 0);
    TEST_ASSERT_EQ(r, AURA_OK);
}

static void test_task_count_after_multi_add_remove(void) {
    sched_setup();
    scheduler_add_task("t1", task_a, 0);
    scheduler_add_task("t2", task_b, 0);
    TEST_ASSERT_EQ(scheduler_task_count(), 2);
    scheduler_remove_task("t1");
    TEST_ASSERT_EQ(scheduler_task_count(), 1);
    scheduler_remove_task("t2");
    TEST_ASSERT_EQ(scheduler_task_count(), 0);
}

/* =========================================================================== */

int main(void) {
    printf("=== test_scheduler ===\n");
    RUN_TEST(test_init_count_zero);
    RUN_TEST(test_add_task_returns_ok);
    RUN_TEST(test_add_task_null_name);
    RUN_TEST(test_add_task_null_fn);
    RUN_TEST(test_add_task_max_slots);
    RUN_TEST(test_remove_task_returns_ok);
    RUN_TEST(test_remove_task_not_found);
    RUN_TEST(test_task_runs_on_tick);
    RUN_TEST(test_task_period_zero_runs_every_tick);
    RUN_TEST(test_task_period_n_respects_period);
    RUN_TEST(test_task_period_fires_multiple_times);
    RUN_TEST(test_zombie_cleaned_up_next_tick);
    RUN_TEST(test_multiple_tasks_all_run);
    RUN_TEST(test_tick_publishes_event);
    RUN_TEST(test_name_truncation);
    RUN_TEST(test_task_count_after_multi_add_remove);
    PRINT_RESULTS();
}
