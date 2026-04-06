/* =============================================================================
 * AI Aura OS — IDT + 8259A PIC Implementation
 * =============================================================================*/

#include "idt.h"
#include "io.h"
#include "vga.h"

/* ---------------------------------------------------------------------------
 * IDT entry (gate descriptor) — 8 bytes each
 * ---------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) {
    uint16_t offset_lo;     /* Bits 0–15 of the handler address  */
    uint16_t selector;      /* Code segment selector (0x08)       */
    uint8_t  zero;          /* Always 0                           */
    uint8_t  type_attr;     /* Gate type + DPL + present bit      */
    uint16_t offset_hi;     /* Bits 16–31 of the handler address  */
} idt_entry_t;

/* IDT pointer loaded with LIDT */
typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} idt_ptr_t;

#define IDT_ENTRIES     256
#define GATE_INT32      0x8E    /* 32-bit interrupt gate, DPL=0, present */
#define KERNEL_CS       0x08    /* Code selector from GDT (see boot.asm) */

static idt_entry_t idt_table[IDT_ENTRIES];
static irq_handler_t irq_handlers[16];

/* ---------------------------------------------------------------------------
 * Low-level ASM stubs — each saves the IRQ number on the stack and calls
 * the common C dispatcher.
 * ---------------------------------------------------------------------------*/
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

/* ---------------------------------------------------------------------------
 * ASM stub definitions — generated with a macro to keep this file concise.
 * Each stub pushes a dummy/real error code and the vector number, then jumps
 * to the shared dispatcher.
 * ---------------------------------------------------------------------------*/

/* Macro: exception with no hardware error code */
#define ISR_NOERR(n)                            \
    __asm__ (                                   \
        ".global isr" #n "\n"                   \
        "isr" #n ":\n"                          \
        "  pushl $0\n"          /* dummy err */ \
        "  pushl $" #n "\n"     /* vector   */ \
        "  jmp isr_common\n"                    \
    )

/* Macro: exception that pushes its own error code */
#define ISR_HWERR(n)                            \
    __asm__ (                                   \
        ".global isr" #n "\n"                   \
        "isr" #n ":\n"                          \
        "  pushl $" #n "\n"     /* vector   */ \
        "  jmp isr_common\n"                    \
    )

/* Macro: hardware IRQ stub */
#define IRQ_STUB(irqn, vec)                     \
    __asm__ (                                   \
        ".global irq" #irqn "\n"               \
        "irq" #irqn ":\n"                      \
        "  pushl $0\n"                          \
        "  pushl $" #vec "\n"                   \
        "  jmp irq_common\n"                    \
    )

ISR_NOERR(0);  ISR_NOERR(1);  ISR_NOERR(2);  ISR_NOERR(3);
ISR_NOERR(4);  ISR_NOERR(5);  ISR_NOERR(6);  ISR_NOERR(7);
ISR_HWERR(8);  ISR_NOERR(9);  ISR_HWERR(10); ISR_HWERR(11);
ISR_HWERR(12); ISR_HWERR(13); ISR_HWERR(14); ISR_NOERR(15);
ISR_NOERR(16); ISR_HWERR(17); ISR_NOERR(18); ISR_NOERR(19);
ISR_NOERR(20); ISR_NOERR(21); ISR_NOERR(22); ISR_NOERR(23);
ISR_NOERR(24); ISR_NOERR(25); ISR_NOERR(26); ISR_NOERR(27);
ISR_NOERR(28); ISR_NOERR(29); ISR_HWERR(30); ISR_NOERR(31);

IRQ_STUB(0,  32); IRQ_STUB(1,  33); IRQ_STUB(2,  34); IRQ_STUB(3,  35);
IRQ_STUB(4,  36); IRQ_STUB(5,  37); IRQ_STUB(6,  38); IRQ_STUB(7,  39);
IRQ_STUB(8,  40); IRQ_STUB(9,  41); IRQ_STUB(10, 42); IRQ_STUB(11, 43);
IRQ_STUB(12, 44); IRQ_STUB(13, 45); IRQ_STUB(14, 46); IRQ_STUB(15, 47);

/* ---------------------------------------------------------------------------
 * Common exception dispatcher (called from ISR stubs)
 * Stack frame at entry: error_code, vector, (pushed by CPU: eip, cs, eflags)
 * ---------------------------------------------------------------------------*/
struct isr_frame {
    uint32_t vector;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
};

__attribute__((used))
static void isr_dispatch(struct isr_frame *f) {
    /* Page fault (#14) — print faulting address from CR2 */
    if (f->vector == 14) {
        uint32_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
        vga_print("\n[CPU] Page Fault at 0x");
        vga_print_hex(cr2);
        vga_print("  eip=0x");
        vga_print_hex(f->eip);
        vga_print("  err=0x");
        vga_print_hex(f->error_code);
        vga_println("");
    } else {
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
        vga_print("\n[CPU] Exception ");
        vga_print_dec(f->vector);
        vga_print("  err=0x");
        vga_print_hex(f->error_code);
        vga_print("  eip=0x");
        vga_print_hex(f->eip);
        vga_println("");
    }
    /* Halt for fatal exceptions */
    __asm__ volatile ("cli; hlt");
}

/* Common ISR trampoline — saves registers, calls dispatcher, restores */
__asm__ (
    "isr_common:\n"
    "  pusha\n"
    /* After pusha, the isr_frame (vector, error_code, eip, cs, eflags) sits
     * at ESP+32 (8 registers × 4 bytes each = 32 bytes of pusha data). */
    "  lea 32(%esp), %eax\n"
    "  push %eax\n"              /* pass pointer to isr_frame */
    "  call isr_dispatch\n"
    "  add $4, %esp\n"
    "  popa\n"
    "  add $8, %esp\n"           /* pop vector + error_code */
    "  iret\n"
);

/* ---------------------------------------------------------------------------
 * Common IRQ dispatcher
 * ---------------------------------------------------------------------------*/
struct irq_frame {
    uint32_t vector;
    uint32_t dummy;
};

__attribute__((used))
static void irq_dispatch(struct irq_frame *f) {
    uint8_t irq = (uint8_t)(f->vector - IRQ_BASE);
    if (irq < 16 && irq_handlers[irq]) {
        irq_handlers[irq]();
    }
    irq_send_eoi(irq);
}

__asm__ (
    "irq_common:\n"
    "  pusha\n"
    /* irq_frame (vector, dummy) is at ESP+32 after pusha saves 8 registers */
    "  lea 32(%esp), %eax\n"
    "  push %eax\n"
    "  call irq_dispatch\n"
    "  add $4, %esp\n"
    "  popa\n"
    "  add $8, %esp\n"
    "  iret\n"
);

/* ---------------------------------------------------------------------------
 * IDT helper: set one gate
 * ---------------------------------------------------------------------------*/
static void idt_set_gate(uint8_t vector, uint32_t handler) {
    idt_table[vector].offset_lo = (uint16_t)(handler & 0xFFFF);
    idt_table[vector].selector  = KERNEL_CS;
    idt_table[vector].zero      = 0;
    idt_table[vector].type_attr = GATE_INT32;
    idt_table[vector].offset_hi = (uint16_t)((handler >> 16) & 0xFFFF);
}

/* ---------------------------------------------------------------------------
 * PIC remapping
 * ---------------------------------------------------------------------------*/
static void pic_remap(void) {
    /* Save current masks */
    uint8_t m1 = inb(PIC1_DATA);
    uint8_t m2 = inb(PIC2_DATA);

    /* Initialise both PICs */
    outb(PIC1_CMD,  PIC_ICW1_INIT); io_wait();
    outb(PIC2_CMD,  PIC_ICW1_INIT); io_wait();

    /* ICW2: set vector offsets */
    outb(PIC1_DATA, IRQ_BASE);       io_wait(); /* IRQ0 → vector 32 */
    outb(PIC2_DATA, IRQ_BASE + 8);   io_wait(); /* IRQ8 → vector 40 */

    /* ICW3: cascade wiring */
    outb(PIC1_DATA, 0x04); io_wait(); /* Master: slave on IRQ2 */
    outb(PIC2_DATA, 0x02); io_wait(); /* Slave: cascade identity 2 */

    /* ICW4: 8086 mode */
    outb(PIC1_DATA, PIC_ICW4_8086); io_wait();
    outb(PIC2_DATA, PIC_ICW4_8086); io_wait();

    /* Restore masks */
    outb(PIC1_DATA, m1);
    outb(PIC2_DATA, m2);
}

/* ---------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------------*/
void idt_init(void) {
    /* Clear handler table */
    for (int i = 0; i < 16; i++) irq_handlers[i] = (void *)0;

    /* Install CPU exception stubs (vectors 0–31) */
    idt_set_gate(0,  (uint32_t)isr0);
    idt_set_gate(1,  (uint32_t)isr1);
    idt_set_gate(2,  (uint32_t)isr2);
    idt_set_gate(3,  (uint32_t)isr3);
    idt_set_gate(4,  (uint32_t)isr4);
    idt_set_gate(5,  (uint32_t)isr5);
    idt_set_gate(6,  (uint32_t)isr6);
    idt_set_gate(7,  (uint32_t)isr7);
    idt_set_gate(8,  (uint32_t)isr8);
    idt_set_gate(9,  (uint32_t)isr9);
    idt_set_gate(10, (uint32_t)isr10);
    idt_set_gate(11, (uint32_t)isr11);
    idt_set_gate(12, (uint32_t)isr12);
    idt_set_gate(13, (uint32_t)isr13);
    idt_set_gate(14, (uint32_t)isr14);
    idt_set_gate(15, (uint32_t)isr15);
    idt_set_gate(16, (uint32_t)isr16);
    idt_set_gate(17, (uint32_t)isr17);
    idt_set_gate(18, (uint32_t)isr18);
    idt_set_gate(19, (uint32_t)isr19);
    idt_set_gate(20, (uint32_t)isr20);
    idt_set_gate(21, (uint32_t)isr21);
    idt_set_gate(22, (uint32_t)isr22);
    idt_set_gate(23, (uint32_t)isr23);
    idt_set_gate(24, (uint32_t)isr24);
    idt_set_gate(25, (uint32_t)isr25);
    idt_set_gate(26, (uint32_t)isr26);
    idt_set_gate(27, (uint32_t)isr27);
    idt_set_gate(28, (uint32_t)isr28);
    idt_set_gate(29, (uint32_t)isr29);
    idt_set_gate(30, (uint32_t)isr30);
    idt_set_gate(31, (uint32_t)isr31);

    /* Install hardware IRQ stubs (vectors 32–47) */
    idt_set_gate(32, (uint32_t)irq0);
    idt_set_gate(33, (uint32_t)irq1);
    idt_set_gate(34, (uint32_t)irq2);
    idt_set_gate(35, (uint32_t)irq3);
    idt_set_gate(36, (uint32_t)irq4);
    idt_set_gate(37, (uint32_t)irq5);
    idt_set_gate(38, (uint32_t)irq6);
    idt_set_gate(39, (uint32_t)irq7);
    idt_set_gate(40, (uint32_t)irq8);
    idt_set_gate(41, (uint32_t)irq9);
    idt_set_gate(42, (uint32_t)irq10);
    idt_set_gate(43, (uint32_t)irq11);
    idt_set_gate(44, (uint32_t)irq12);
    idt_set_gate(45, (uint32_t)irq13);
    idt_set_gate(46, (uint32_t)irq14);
    idt_set_gate(47, (uint32_t)irq15);

    /* Load the IDT */
    idt_ptr_t ptr;
    ptr.limit = (uint16_t)(sizeof(idt_table) - 1);
    ptr.base  = (uint32_t)idt_table;
    __asm__ volatile ("lidt %0" : : "m"(ptr));

    /* Remap the 8259A PIC so IRQs land above CPU exceptions */
    pic_remap();

    /* Mask all IRQs initially — subsystems unmask their own IRQ after init */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    /* Enable hardware interrupts */
    __asm__ volatile ("sti");
}

void irq_register_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16) irq_handlers[irq] = handler;
}

void irq_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

void irq_mask(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t  bit  = (irq < 8) ? irq : (uint8_t)(irq - 8);
    outb(port, (uint8_t)(inb(port) | (1u << bit)));
}

void irq_unmask(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t  bit  = (irq < 8) ? irq : (uint8_t)(irq - 8);
    outb(port, (uint8_t)(inb(port) & ~(1u << bit)));
}
