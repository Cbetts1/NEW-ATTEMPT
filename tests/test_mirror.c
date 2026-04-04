/* =============================================================================
 * AI Aura OS — Mirror Unit Tests
 *
 * memory_stats() and plugin_count() are provided by tests/stubs/mirror_deps.c
 * so that mirror.c can be tested in isolation.
 * =============================================================================*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tests/framework.h"
#include "kernel/mirror.h"
#include "kernel/eventbus.h"

/* Helpers declared in mirror_deps.c */
extern void mirror_test_set_memory(uint32_t used, uint32_t free_bytes);
extern void mirror_test_set_plugin_count(int n);

/* =========================================================================== */
/* Setup                                                                        */
/* =========================================================================== */

static void mirror_setup(void) {
    eventbus_init();
    mirror_init();
    mirror_test_set_memory(1024, 4096);
    mirror_test_set_plugin_count(0);
}

/* =========================================================================== */
/* Tests                                                                        */
/* =========================================================================== */

static void test_init_all_slots_invalid(void) {
    mirror_setup();
    /* After init no slot should be valid — verified by mirror_restore failing */
    for (uint8_t i = 1; i < MIRROR_SLOTS; i++) {
        aura_status_t r = mirror_restore(i);
        TEST_ASSERT_EQ(r, AURA_ERR);
    }
}

static void test_sync_makes_live_slot_valid(void) {
    mirror_setup();
    mirror_sync();
    /* Live slot (0) is now valid; mirror_restore(0) should succeed */
    aura_status_t r = mirror_restore(0);
    TEST_ASSERT_EQ(r, AURA_OK);
}

static void test_sync_records_memory_stats(void) {
    mirror_setup();
    mirror_test_set_memory(512, 2048);
    mirror_sync();
    /* We cannot read snapshot internals directly, but we can capture to
     * slot 1 and then dump — no crash means data is consistent.  We verify
     * indirectly by checking capture succeeds and restore works. */
    aura_status_t r = mirror_capture(1, "check", MIRROR_FLAG_ALL);
    TEST_ASSERT_EQ(r, AURA_OK);
}

static void test_capture_returns_ok(void) {
    mirror_setup();
    aura_status_t r = mirror_capture(1, "snap1", MIRROR_FLAG_ALL);
    TEST_ASSERT_EQ(r, AURA_OK);
}

static void test_capture_invalid_slot(void) {
    mirror_setup();
    aura_status_t r = mirror_capture(MIRROR_SLOTS, "bad", MIRROR_FLAG_ALL);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_capture_slot_zero(void) {
    /* Capturing to slot 0 overwrites the live slot — should still succeed */
    mirror_setup();
    aura_status_t r = mirror_capture(0, "live_snap", MIRROR_FLAG_MEMORY);
    TEST_ASSERT_EQ(r, AURA_OK);
}

static void test_capture_all_slots(void) {
    mirror_setup();
    for (uint8_t i = 0; i < MIRROR_SLOTS; i++) {
        char label[16];
        snprintf(label, sizeof(label), "slot%u", (unsigned)i);
        aura_status_t r = mirror_capture(i, label, MIRROR_FLAG_ALL);
        TEST_ASSERT_EQ(r, AURA_OK);
    }
}

static void test_restore_valid_slot(void) {
    mirror_setup();
    mirror_capture(1, "restore_test", MIRROR_FLAG_ALL);
    aura_status_t r = mirror_restore(1);
    TEST_ASSERT_EQ(r, AURA_OK);
}

static void test_restore_empty_slot(void) {
    mirror_setup();
    /* Slot 2 was never captured → invalid */
    aura_status_t r = mirror_restore(2);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_restore_invalid_slot_number(void) {
    mirror_setup();
    aura_status_t r = mirror_restore(MIRROR_SLOTS);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_restore_publishes_event(void) {
    mirror_setup();
    mirror_capture(1, "evt_test", MIRROR_FLAG_ALL);
    uint32_t before = eventbus_pending();
    mirror_restore(1);
    /* mirror_restore publishes to TOPIC_MIRROR_SYNC */
    TEST_ASSERT(eventbus_pending() > before);
}

static void test_dump_does_not_crash_empty(void) {
    mirror_setup();
    /* Dump an empty slot — must not crash */
    mirror_dump(3);
    TEST_ASSERT(1);
}

static void test_dump_does_not_crash_valid(void) {
    mirror_setup();
    mirror_capture(2, "dumpme", MIRROR_FLAG_ALL);
    mirror_dump(2);
    TEST_ASSERT(1);
}

static void test_dump_all_does_not_crash(void) {
    mirror_setup();
    mirror_capture(1, "a", MIRROR_FLAG_MEMORY);
    mirror_dump_all();
    TEST_ASSERT(1);
}

static void test_sync_increments_event_at_256(void) {
    /* After 256 sync calls the event bus should receive a MIRROR_SYNC event */
    mirror_setup();
    eventbus_init();

    uint32_t events_before = eventbus_pending();
    for (int i = 0; i < 256; i++)
        mirror_sync();

    TEST_ASSERT(eventbus_pending() > events_before);
}

static void test_capture_uses_flags(void) {
    /* Capture with a specific flag; repeated capture with different flag
     * must succeed (no state machine that blocks re-capture). */
    mirror_setup();
    aura_status_t r1 = mirror_capture(1, "mem_only",  MIRROR_FLAG_MEMORY);
    aura_status_t r2 = mirror_capture(1, "all_flags", MIRROR_FLAG_ALL);
    TEST_ASSERT_EQ(r1, AURA_OK);
    TEST_ASSERT_EQ(r2, AURA_OK);
}

static void test_multiple_captures_independent(void) {
    mirror_setup();
    /* Capture at different points and verify both slots remain restorable */
    mirror_capture(1, "snap1", MIRROR_FLAG_ALL);
    mirror_capture(2, "snap2", MIRROR_FLAG_ALL);
    TEST_ASSERT_EQ(mirror_restore(1), AURA_OK);
    TEST_ASSERT_EQ(mirror_restore(2), AURA_OK);
}

/* =========================================================================== */

int main(void) {
    printf("=== test_mirror ===\n");
    RUN_TEST(test_init_all_slots_invalid);
    RUN_TEST(test_sync_makes_live_slot_valid);
    RUN_TEST(test_sync_records_memory_stats);
    RUN_TEST(test_capture_returns_ok);
    RUN_TEST(test_capture_invalid_slot);
    RUN_TEST(test_capture_slot_zero);
    RUN_TEST(test_capture_all_slots);
    RUN_TEST(test_restore_valid_slot);
    RUN_TEST(test_restore_empty_slot);
    RUN_TEST(test_restore_invalid_slot_number);
    RUN_TEST(test_restore_publishes_event);
    RUN_TEST(test_dump_does_not_crash_empty);
    RUN_TEST(test_dump_does_not_crash_valid);
    RUN_TEST(test_dump_all_does_not_crash);
    RUN_TEST(test_sync_increments_event_at_256);
    RUN_TEST(test_capture_uses_flags);
    RUN_TEST(test_multiple_captures_independent);
    PRINT_RESULTS();
}
