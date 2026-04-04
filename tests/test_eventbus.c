/* =============================================================================
 * AI Aura OS — Event Bus Unit Tests
 * =============================================================================*/

#include <stdio.h>
#include <stdint.h>

#include "tests/framework.h"
#include "kernel/eventbus.h"

/* =========================================================================== */
/* Shared handler state for callbacks                                           */
/* =========================================================================== */

static int      s_call_count   = 0;
static uint8_t  s_last_topic   = 0xFF;
static uint32_t s_last_data    = 0;
static int      s_handler2_calls = 0;

static void reset_handler_state(void) {
    s_call_count    = 0;
    s_last_topic    = 0xFF;
    s_last_data     = 0;
    s_handler2_calls = 0;
}

static void handler1(const aura_event_t *evt) {
    s_call_count++;
    s_last_topic = evt->topic;
    s_last_data  = evt->data;
}

static void handler2(const aura_event_t *evt) {
    (void)evt;
    s_handler2_calls++;
}

/* =========================================================================== */
/* Tests                                                                        */
/* =========================================================================== */

static void test_init_clears_state(void) {
    eventbus_init();
    TEST_ASSERT_EQ(eventbus_pending(), 0u);
}

static void test_subscribe_returns_ok(void) {
    eventbus_init();
    aura_status_t r = eventbus_subscribe(TOPIC_KERNEL_TICK, handler1);
    TEST_ASSERT_EQ(r, AURA_OK);
}

static void test_subscribe_bad_topic(void) {
    eventbus_init();
    aura_status_t r = eventbus_subscribe(EVENTBUS_MAX_TOPICS, handler1);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_subscribe_null_handler(void) {
    eventbus_init();
    aura_status_t r = eventbus_subscribe(TOPIC_KERNEL_TICK, NULL);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_subscribe_fills_all_slots(void) {
    eventbus_init();
    /* Fill every handler slot for TOPIC_USER_INPUT */
    for (int i = 0; i < EVENTBUS_MAX_HANDLERS; i++) {
        aura_status_t r = eventbus_subscribe(TOPIC_USER_INPUT, handler1);
        TEST_ASSERT_EQ(r, AURA_OK);
    }
    /* One more must fail */
    aura_status_t r = eventbus_subscribe(TOPIC_USER_INPUT, handler1);
    TEST_ASSERT_EQ(r, AURA_NOSLOT);
}

static void test_unsubscribe_returns_ok(void) {
    eventbus_init();
    eventbus_subscribe(TOPIC_KERNEL_TICK, handler1);
    aura_status_t r = eventbus_unsubscribe(TOPIC_KERNEL_TICK, handler1);
    TEST_ASSERT_EQ(r, AURA_OK);
}

static void test_unsubscribe_not_found(void) {
    eventbus_init();
    aura_status_t r = eventbus_unsubscribe(TOPIC_KERNEL_TICK, handler1);
    TEST_ASSERT_EQ(r, AURA_NOTFOUND);
}

static void test_unsubscribe_bad_topic(void) {
    eventbus_init();
    aura_status_t r = eventbus_unsubscribe(EVENTBUS_MAX_TOPICS, handler1);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_unsubscribe_null_handler(void) {
    eventbus_init();
    aura_status_t r = eventbus_unsubscribe(TOPIC_KERNEL_TICK, NULL);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_publish_increments_pending(void) {
    eventbus_init();
    TEST_ASSERT_EQ(eventbus_pending(), 0u);
    eventbus_publish(TOPIC_KERNEL_TICK, 0);
    TEST_ASSERT_EQ(eventbus_pending(), 1u);
    eventbus_publish(TOPIC_KERNEL_TICK, 0);
    TEST_ASSERT_EQ(eventbus_pending(), 2u);
}

static void test_publish_bad_topic(void) {
    eventbus_init();
    aura_status_t r = eventbus_publish(EVENTBUS_MAX_TOPICS, 0);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_publish_queue_overflow(void) {
    eventbus_init();
    for (int i = 0; i < EVENTBUS_QUEUE_DEPTH; i++) {
        aura_status_t r = eventbus_publish(TOPIC_KERNEL_TICK, (uint32_t)i);
        TEST_ASSERT_EQ(r, AURA_OK);
    }
    /* Next publish should fail with AURA_BUSY */
    aura_status_t r = eventbus_publish(TOPIC_KERNEL_TICK, 0);
    TEST_ASSERT_EQ(r, AURA_BUSY);
}

static void test_process_invokes_handler(void) {
    eventbus_init();
    reset_handler_state();
    eventbus_subscribe(TOPIC_KERNEL_TICK, handler1);
    eventbus_publish(TOPIC_KERNEL_TICK, 42u);
    TEST_ASSERT_EQ(eventbus_pending(), 1u);
    eventbus_process();
    TEST_ASSERT_EQ(s_call_count, 1);
    TEST_ASSERT_EQ(s_last_topic, TOPIC_KERNEL_TICK);
    TEST_ASSERT_EQ(s_last_data, 42u);
}

static void test_process_clears_pending(void) {
    eventbus_init();
    eventbus_subscribe(TOPIC_KERNEL_TICK, handler1);
    eventbus_publish(TOPIC_KERNEL_TICK, 0);
    eventbus_process();
    TEST_ASSERT_EQ(eventbus_pending(), 0u);
}

static void test_process_multiple_handlers(void) {
    eventbus_init();
    reset_handler_state();
    eventbus_subscribe(TOPIC_PANIC, handler1);
    eventbus_subscribe(TOPIC_PANIC, handler2);
    eventbus_publish(TOPIC_PANIC, 0);
    eventbus_process();
    TEST_ASSERT_EQ(s_call_count,    1);
    TEST_ASSERT_EQ(s_handler2_calls, 1);
}

static void test_unsubscribed_handler_not_called(void) {
    eventbus_init();
    reset_handler_state();
    eventbus_subscribe(TOPIC_KERNEL_TICK, handler1);
    eventbus_unsubscribe(TOPIC_KERNEL_TICK, handler1);
    eventbus_publish(TOPIC_KERNEL_TICK, 0);
    eventbus_process();
    TEST_ASSERT_EQ(s_call_count, 0);
}

static void test_no_handler_for_topic(void) {
    /* Publish to a topic with no subscribers — must not crash */
    eventbus_init();
    reset_handler_state();
    eventbus_publish(TOPIC_MIRROR_SYNC, 99u);
    eventbus_process();
    TEST_ASSERT_EQ(s_call_count, 0);
}

static void test_circular_queue_wrap(void) {
    /* Fill the queue, drain it, then fill half again — exercises the wrap. */
    eventbus_init();
    reset_handler_state();
    eventbus_subscribe(TOPIC_KERNEL_TICK, handler1);

    for (int i = 0; i < EVENTBUS_QUEUE_DEPTH; i++)
        eventbus_publish(TOPIC_KERNEL_TICK, (uint32_t)i);

    eventbus_process();
    TEST_ASSERT_EQ(s_call_count, EVENTBUS_QUEUE_DEPTH);
    TEST_ASSERT_EQ(eventbus_pending(), 0u);

    /* Publish again after full drain */
    reset_handler_state();
    eventbus_publish(TOPIC_KERNEL_TICK, 77u);
    eventbus_process();
    TEST_ASSERT_EQ(s_call_count, 1);
    TEST_ASSERT_EQ(s_last_data,  77u);
}

static void test_multiple_topics_independent(void) {
    eventbus_init();
    reset_handler_state();
    eventbus_subscribe(TOPIC_KERNEL_TICK, handler1);
    /* Do NOT subscribe to TOPIC_USER_INPUT */
    eventbus_publish(TOPIC_USER_INPUT, 0);
    eventbus_process();
    TEST_ASSERT_EQ(s_call_count, 0); /* handler1 not called for USER_INPUT */

    eventbus_publish(TOPIC_KERNEL_TICK, 1u);
    eventbus_process();
    TEST_ASSERT_EQ(s_call_count, 1);
}

/* =========================================================================== */

int main(void) {
    printf("=== test_eventbus ===\n");
    RUN_TEST(test_init_clears_state);
    RUN_TEST(test_subscribe_returns_ok);
    RUN_TEST(test_subscribe_bad_topic);
    RUN_TEST(test_subscribe_null_handler);
    RUN_TEST(test_subscribe_fills_all_slots);
    RUN_TEST(test_unsubscribe_returns_ok);
    RUN_TEST(test_unsubscribe_not_found);
    RUN_TEST(test_unsubscribe_bad_topic);
    RUN_TEST(test_unsubscribe_null_handler);
    RUN_TEST(test_publish_increments_pending);
    RUN_TEST(test_publish_bad_topic);
    RUN_TEST(test_publish_queue_overflow);
    RUN_TEST(test_process_invokes_handler);
    RUN_TEST(test_process_clears_pending);
    RUN_TEST(test_process_multiple_handlers);
    RUN_TEST(test_unsubscribed_handler_not_called);
    RUN_TEST(test_no_handler_for_topic);
    RUN_TEST(test_circular_queue_wrap);
    RUN_TEST(test_multiple_topics_independent);
    PRINT_RESULTS();
}
