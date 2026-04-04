/* =============================================================================
 * AI Aura OS — Kernel Globals Stub for Host-Side Tests
 *
 * Provides definitions for the global kernel variables that are declared
 * `extern` in kernel.h and referenced by eventbus, plugin, mirror, etc.
 * =============================================================================*/

#include <stdint.h>
#include "kernel.h"

volatile uint32_t       g_tick_count    = 0;
volatile kernel_state_t g_kernel_state  = KERNEL_STATE_BOOT;
