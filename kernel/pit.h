#ifndef PIT_H
#define PIT_H

/* =============================================================================
 * AI Aura OS — Programmable Interval Timer (8253/8254) Driver
 *
 * Configures PIT channel 0 (IRQ0) to fire at a fixed frequency.
 * The IRQ0 handler increments g_tick_count so that tick timing is consistent
 * and independent of the scheduler loop speed.
 * =============================================================================*/

#include <stdint.h>

/* Desired tick frequency in Hz */
#define PIT_FREQUENCY_HZ    100u

void pit_init(void);

#endif /* PIT_H */
