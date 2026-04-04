/* =============================================================================
 * AI Aura OS — Mirror Dependency Stubs for Host-Side Tests
 *
 * Provides controllable stubs for memory_stats() and plugin_count() so that
 * mirror.c can be unit-tested in isolation without the real memory/plugin
 * subsystems.
 * =============================================================================*/

#include <stdint.h>
#include "memory.h"
#include "plugin.h"

/* Backing values; tests can adjust them via the helper setters below. */
static uint32_t s_mem_used        = 1024;
static uint32_t s_mem_free        = 4096;
static int      s_plugin_count_val = 0;

/* --- Stub implementations -------------------------------------------------- */

void memory_stats(uint32_t *used, uint32_t *free_bytes) {
    *used       = s_mem_used;
    *free_bytes = s_mem_free;
}

int plugin_count(void) {
    return s_plugin_count_val;
}

/* --- Test helpers ----------------------------------------------------------- */

void mirror_test_set_memory(uint32_t used, uint32_t free_bytes) {
    s_mem_used  = used;
    s_mem_free  = free_bytes;
}

void mirror_test_set_plugin_count(int n) {
    s_plugin_count_val = n;
}
