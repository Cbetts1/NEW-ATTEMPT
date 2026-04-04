/* =============================================================================
 * AI Aura OS — Plugin Manager Unit Tests
 * =============================================================================*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tests/framework.h"
#include "kernel/plugin.h"
#include "kernel/eventbus.h"

/* =========================================================================== */
/* Helper plugin callbacks                                                      */
/* =========================================================================== */

static int s_init_calls     = 0;
static int s_tick_calls     = 0;
static int s_shutdown_calls = 0;
static int s_init_should_fail = 0;
static int s_err_tick_calls   = 0;

static aura_status_t plug_init(void)     { s_init_calls++;     return s_init_should_fail ? AURA_ERR : AURA_OK; }
static aura_status_t plug_tick(void)     { s_tick_calls++;     return AURA_OK; }
static aura_status_t plug_shutdown(void) { s_shutdown_calls++; return AURA_OK; }
static aura_status_t err_tick(void)      { s_err_tick_calls++; return AURA_ERR; }

static void reset_callbacks(void) {
    s_init_calls      = 0;
    s_tick_calls      = 0;
    s_shutdown_calls  = 0;
    s_init_should_fail = 0;
}

/* Build a minimal plugin descriptor. */
static plugin_desc_t make_desc(const char *name) {
    plugin_desc_t d;
    memset(&d, 0, sizeof(d));
    strncpy(d.name, name, PLUGIN_NAME_LEN - 1);
    strncpy(d.version, "1.0", PLUGIN_VERSION_LEN - 1);
    d.type     = PLUGIN_TYPE_SERVICE;
    d.init     = plug_init;
    d.tick     = plug_tick;
    d.shutdown = plug_shutdown;
    return d;
}

/* Initialize both subsystems before each test. */
static void plug_setup(void) {
    eventbus_init();
    plugin_manager_init();
    reset_callbacks();
}

/* =========================================================================== */
/* Tests                                                                        */
/* =========================================================================== */

static void test_init_count_zero(void) {
    plug_setup();
    TEST_ASSERT_EQ(plugin_count(), 0);
}

static void test_register_returns_ok(void) {
    plug_setup();
    plugin_desc_t d = make_desc("alpha");
    aura_status_t r = plugin_register(&d);
    TEST_ASSERT_EQ(r, AURA_OK);
    TEST_ASSERT_EQ(plugin_count(), 1);
}

static void test_register_null_desc(void) {
    plug_setup();
    aura_status_t r = plugin_register(NULL);
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_register_duplicate_name(void) {
    plug_setup();
    plugin_desc_t d = make_desc("dup");
    plugin_register(&d);
    aura_status_t r = plugin_register(&d); /* same name again */
    TEST_ASSERT_EQ(r, AURA_ERR);
    TEST_ASSERT_EQ(plugin_count(), 1);
}

static void test_register_max_plugins(void) {
    plug_setup();
    char name[PLUGIN_NAME_LEN];
    for (int i = 0; i < PLUGIN_MAX; i++) {
        snprintf(name, sizeof(name), "plug%d", i);
        plugin_desc_t d = make_desc(name);
        aura_status_t r = plugin_register(&d);
        TEST_ASSERT_EQ(r, AURA_OK);
    }
    TEST_ASSERT_EQ(plugin_count(), PLUGIN_MAX);
    /* One more must fail */
    plugin_desc_t d = make_desc("overflow");
    aura_status_t r = plugin_register(&d);
    TEST_ASSERT_EQ(r, AURA_NOSLOT);
}

static void test_unregister_returns_ok(void) {
    plug_setup();
    plugin_desc_t d = make_desc("beta");
    plugin_register(&d);
    aura_status_t r = plugin_unregister("beta");
    TEST_ASSERT_EQ(r, AURA_OK);
    TEST_ASSERT_EQ(plugin_count(), 0);
}

static void test_unregister_not_found(void) {
    plug_setup();
    aura_status_t r = plugin_unregister("ghost");
    TEST_ASSERT_EQ(r, AURA_NOTFOUND);
}

static void test_activate_calls_init(void) {
    plug_setup();
    plugin_desc_t d = make_desc("init_test");
    plugin_register(&d);
    aura_status_t r = plugin_activate("init_test");
    TEST_ASSERT_EQ(r, AURA_OK);
    TEST_ASSERT_EQ(s_init_calls, 1);
}

static void test_activate_state_becomes_active(void) {
    plug_setup();
    plugin_desc_t d = make_desc("active_test");
    plugin_register(&d);
    plugin_activate("active_test");
    plugin_slot_t *s = plugin_find("active_test");
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQ(s->state, PLUGIN_STATE_ACTIVE);
}

static void test_activate_not_found(void) {
    plug_setup();
    aura_status_t r = plugin_activate("nonexistent");
    TEST_ASSERT_EQ(r, AURA_NOTFOUND);
}

static void test_activate_wrong_state(void) {
    /* A plugin that's already ACTIVE cannot be activated again */
    plug_setup();
    plugin_desc_t d = make_desc("state_test");
    plugin_register(&d);
    plugin_activate("state_test");
    aura_status_t r = plugin_activate("state_test"); /* already ACTIVE */
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_activate_init_failure_sets_error(void) {
    plug_setup();
    s_init_should_fail = 1;
    plugin_desc_t d = make_desc("fail_init");
    plugin_register(&d);
    aura_status_t r = plugin_activate("fail_init");
    TEST_ASSERT(r != AURA_OK);

    plugin_slot_t *s = plugin_find("fail_init");
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQ(s->state, PLUGIN_STATE_ERROR);
    TEST_ASSERT(s->error_count > 0u);
}

static void test_deactivate_returns_ok(void) {
    plug_setup();
    plugin_desc_t d = make_desc("deact_test");
    plugin_register(&d);
    plugin_activate("deact_test");
    aura_status_t r = plugin_deactivate("deact_test");
    TEST_ASSERT_EQ(r, AURA_OK);
}

static void test_deactivate_calls_shutdown(void) {
    plug_setup();
    plugin_desc_t d = make_desc("shutdown_test");
    plugin_register(&d);
    plugin_activate("shutdown_test");
    plugin_deactivate("shutdown_test");
    TEST_ASSERT_EQ(s_shutdown_calls, 1);
}

static void test_deactivate_state_becomes_loaded(void) {
    plug_setup();
    plugin_desc_t d = make_desc("deact_state");
    plugin_register(&d);
    plugin_activate("deact_state");
    plugin_deactivate("deact_state");
    plugin_slot_t *s = plugin_find("deact_state");
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQ(s->state, PLUGIN_STATE_LOADED);
}

static void test_deactivate_not_active(void) {
    plug_setup();
    plugin_desc_t d = make_desc("not_active");
    plugin_register(&d); /* state = LOADED */
    aura_status_t r = plugin_deactivate("not_active");
    TEST_ASSERT_EQ(r, AURA_ERR);
}

static void test_tick_all_calls_active_plugins(void) {
    plug_setup();
    plugin_desc_t d = make_desc("ticker");
    plugin_register(&d);
    plugin_activate("ticker");
    plugin_tick_all();
    TEST_ASSERT_EQ(s_tick_calls, 1);
    plugin_tick_all();
    TEST_ASSERT_EQ(s_tick_calls, 2);
}

static void test_tick_all_skips_inactive(void) {
    plug_setup();
    plugin_desc_t d = make_desc("inactive_tick");
    plugin_register(&d); /* LOADED, not ACTIVE */
    plugin_tick_all();
    TEST_ASSERT_EQ(s_tick_calls, 0);
}

static void test_tick_error_increments_error_count(void) {
    plug_setup();
    /* Give the plugin a tick that returns an error */
    plugin_desc_t d;
    memset(&d, 0, sizeof(d));
    strncpy(d.name, "err_tick", PLUGIN_NAME_LEN - 1);
    strncpy(d.version, "0.1", PLUGIN_VERSION_LEN - 1);
    d.type     = PLUGIN_TYPE_SERVICE;
    d.init     = plug_init;
    d.tick     = err_tick;
    d.shutdown = plug_shutdown;
    plugin_register(&d);
    plugin_activate("err_tick");
    plugin_tick_all();

    plugin_slot_t *s = plugin_find("err_tick");
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT(s->error_count > 0u);
    TEST_ASSERT(s->tick_count  > 0u);
}

static void test_find_existing(void) {
    plug_setup();
    plugin_desc_t d = make_desc("findme");
    plugin_register(&d);
    plugin_slot_t *s = plugin_find("findme");
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT(strncmp(s->desc.name, "findme", PLUGIN_NAME_LEN) == 0);
}

static void test_find_not_found(void) {
    plug_setup();
    plugin_slot_t *s = plugin_find("nobody");
    TEST_ASSERT_NULL(s);
}

static void test_unregister_active_calls_shutdown(void) {
    plug_setup();
    plugin_desc_t d = make_desc("kill_active");
    plugin_register(&d);
    plugin_activate("kill_active");
    plugin_unregister("kill_active");
    TEST_ASSERT_EQ(s_shutdown_calls, 1);
}

static void test_count_correct(void) {
    plug_setup();
    TEST_ASSERT_EQ(plugin_count(), 0);
    plugin_desc_t d1 = make_desc("one");
    plugin_desc_t d2 = make_desc("two");
    plugin_register(&d1);
    TEST_ASSERT_EQ(plugin_count(), 1);
    plugin_register(&d2);
    TEST_ASSERT_EQ(plugin_count(), 2);
    plugin_unregister("one");
    TEST_ASSERT_EQ(plugin_count(), 1);
}

static void test_no_callbacks_are_ok(void) {
    /* Plugin with NULL init/tick/shutdown must not crash */
    plug_setup();
    plugin_desc_t d;
    memset(&d, 0, sizeof(d));
    strncpy(d.name, "nocb", PLUGIN_NAME_LEN - 1);
    strncpy(d.version, "0.1", PLUGIN_VERSION_LEN - 1);
    d.type     = PLUGIN_TYPE_CORE;
    d.init     = NULL;
    d.tick     = NULL;
    d.shutdown = NULL;
    plugin_register(&d);
    plugin_activate("nocb");
    plugin_tick_all();
    plugin_deactivate("nocb");
    plugin_unregister("nocb");
    TEST_ASSERT_EQ(plugin_count(), 0);
}

/* =========================================================================== */

int main(void) {
    printf("=== test_plugin ===\n");
    RUN_TEST(test_init_count_zero);
    RUN_TEST(test_register_returns_ok);
    RUN_TEST(test_register_null_desc);
    RUN_TEST(test_register_duplicate_name);
    RUN_TEST(test_register_max_plugins);
    RUN_TEST(test_unregister_returns_ok);
    RUN_TEST(test_unregister_not_found);
    RUN_TEST(test_activate_calls_init);
    RUN_TEST(test_activate_state_becomes_active);
    RUN_TEST(test_activate_not_found);
    RUN_TEST(test_activate_wrong_state);
    RUN_TEST(test_activate_init_failure_sets_error);
    RUN_TEST(test_deactivate_returns_ok);
    RUN_TEST(test_deactivate_calls_shutdown);
    RUN_TEST(test_deactivate_state_becomes_loaded);
    RUN_TEST(test_deactivate_not_active);
    RUN_TEST(test_tick_all_calls_active_plugins);
    RUN_TEST(test_tick_all_skips_inactive);
    RUN_TEST(test_tick_error_increments_error_count);
    RUN_TEST(test_find_existing);
    RUN_TEST(test_find_not_found);
    RUN_TEST(test_unregister_active_calls_shutdown);
    RUN_TEST(test_count_correct);
    RUN_TEST(test_no_callbacks_are_ok);
    PRINT_RESULTS();
}
