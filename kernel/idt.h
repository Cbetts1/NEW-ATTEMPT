#ifndef IDT_H
#define IDT_H

/* =============================================================================
 * AI Aura OS — Interrupt Descriptor Table (IDT) + 8259A PIC
 *
 * Sets up a 256-entry IDT for x86 protected mode:
 *   - Vectors  0–31  : CPU exception handlers
 *   - Vectors 32–47  : Hardware IRQs 0–15 (remapped from default 0x08/0x70)
 *   - Vectors 48–255 : Reserved / future use
 *
 * The 8259A master PIC is remapped so that IRQ0 starts at vector 0x20 (32),
 * avoiding conflicts with x86 CPU exception vectors (0–31).
 * =============================================================================*/

#include <stdint.h>

/* IRQ base vector after PIC remapping */
#define IRQ_BASE        0x20    /* IRQ0 = vector 32, IRQ15 = vector 47 */
#define IRQ_KEYBOARD    1       /* IRQ line number (before base offset)  */
#define IRQ_TIMER       0       /* IRQ line number for PIT channel 0     */

/* 8259A PIC I/O ports */
#define PIC1_CMD        0x20
#define PIC1_DATA       0x21
#define PIC2_CMD        0xA0
#define PIC2_DATA       0xA1

/* PIC commands */
#define PIC_EOI         0x20    /* End-of-interrupt command */
#define PIC_ICW1_INIT   0x11    /* Initialise + ICW4 needed */
#define PIC_ICW4_8086   0x01    /* 8086 mode */

/* IRQ handler callback type */
typedef void (*irq_handler_t)(void);

/* Initialise the IDT, remap the PIC, and enable interrupts */
void idt_init(void);

/* Register a C-level handler for a hardware IRQ line (0–15) */
void irq_register_handler(uint8_t irq, irq_handler_t handler);

/* Send EOI to the appropriate PIC(s) for the given IRQ line (0–15) */
void irq_send_eoi(uint8_t irq);

/* Mask / unmask a specific IRQ line */
void irq_mask(uint8_t irq);
void irq_unmask(uint8_t irq);

#endif /* IDT_H */
