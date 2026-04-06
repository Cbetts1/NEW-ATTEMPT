/* =============================================================================
 * AI Aura OS — PIT (8253/8254) Implementation
 * =============================================================================*/

#include "pit.h"
#include "idt.h"
#include "io.h"
#include "kernel.h"

/* PIT I/O ports */
#define PIT_CHANNEL0    0x40
#define PIT_COMMAND     0x43

/* Command byte: channel 0, lobyte/hibyte, mode 3 (square wave), binary */
#define PIT_CMD_CH0     0x36

/* PIT input clock frequency */
#define PIT_BASE_HZ     1193182u

/* ---------------------------------------------------------------------------
 * IRQ0 handler — fires at PIT_FREQUENCY_HZ ticks per second
 * ---------------------------------------------------------------------------*/
static void pit_irq_handler(void) {
    /* Increment the global kernel tick counter used by the scheduler and
     * the mirror engine.  The extern declaration resolves to kernel.c. */
    extern volatile uint32_t g_tick_count;
    g_tick_count++;
}

/* ---------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------------*/
void pit_init(void) {
    /* Calculate the reload divisor for the desired frequency.
     * PIT_BASE_HZ (1193182) / PIT_FREQUENCY_HZ (100) = 11931, which fits
     * in a 16-bit value (11931 < 65535), so the cap below is a safety net
     * for any future changes to PIT_FREQUENCY_HZ that lower it significantly. */
    uint32_t divisor = PIT_BASE_HZ / PIT_FREQUENCY_HZ;
    if (divisor > 0xFFFFu) divisor = 0xFFFFu;

    /* Program PIT channel 0 */
    outb(PIT_COMMAND, PIT_CMD_CH0);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));        /* low byte  */
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF)); /* high byte */

    /* Register the IRQ0 handler and unmask the timer line */
    irq_register_handler(IRQ_TIMER, pit_irq_handler);
    irq_unmask(IRQ_TIMER);
}
