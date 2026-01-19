/*
 * NanoSec OS - Interrupt Descriptor Table (IDT)
 * ===============================================
 * Handles CPU exceptions and hardware interrupts
 */

#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>

/* IDT entry (gate descriptor) */
typedef struct {
  uint16_t base_low;  /* Lower 16 bits of handler address */
  uint16_t selector;  /* Kernel code segment selector */
  uint8_t zero;       /* Always 0 */
  uint8_t flags;      /* Type and attributes */
  uint16_t base_high; /* Upper 16 bits of handler address */
} __attribute__((packed)) idt_entry_t;

/* IDTR register structure */
typedef struct {
  uint16_t limit; /* Size of IDT - 1 */
  uint32_t base;  /* Base address of IDT */
} __attribute__((packed)) idt_ptr_t;

/* Interrupt frame pushed by CPU */
typedef struct {
  uint32_t ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t int_no, err_code;
  uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed)) interrupt_frame_t;

/* Interrupt handler type */
typedef void (*isr_handler_t)(interrupt_frame_t *frame);

/* IDT flags */
#define IDT_PRESENT 0x80
#define IDT_DPL_RING0 0x00
#define IDT_DPL_RING3 0x60
#define IDT_GATE_INT 0x0E  /* 32-bit interrupt gate */
#define IDT_GATE_TRAP 0x0F /* 32-bit trap gate */

/* Standard IDT flags for kernel interrupt */
#define IDT_FLAGS_KERNEL (IDT_PRESENT | IDT_DPL_RING0 | IDT_GATE_INT)
#define IDT_FLAGS_USER (IDT_PRESENT | IDT_DPL_RING3 | IDT_GATE_INT)

/* ISR numbers */
#define ISR_DIVIDE_ERROR 0
#define ISR_DEBUG 1
#define ISR_NMI 2
#define ISR_BREAKPOINT 3
#define ISR_OVERFLOW 4
#define ISR_BOUND_RANGE 5
#define ISR_INVALID_OPCODE 6
#define ISR_NO_COPROCESSOR 7
#define ISR_DOUBLE_FAULT 8
#define ISR_COPROCESSOR 9
#define ISR_INVALID_TSS 10
#define ISR_SEGMENT_MISSING 11
#define ISR_STACK_FAULT 12
#define ISR_GPF 13
#define ISR_PAGE_FAULT 14
#define ISR_RESERVED 15
#define ISR_FPU_ERROR 16
#define ISR_ALIGN_CHECK 17
#define ISR_MACHINE_CHECK 18
#define ISR_SIMD_ERROR 19

/* IRQ numbers (remapped to 32-47) */
#define IRQ0 32  /* Timer */
#define IRQ1 33  /* Keyboard */
#define IRQ2 34  /* Cascade */
#define IRQ3 35  /* COM2 */
#define IRQ4 36  /* COM1 */
#define IRQ5 37  /* LPT2 */
#define IRQ6 38  /* Floppy */
#define IRQ7 39  /* LPT1 */
#define IRQ8 40  /* RTC */
#define IRQ9 41  /* Free */
#define IRQ10 42 /* Free */
#define IRQ11 43 /* Free */
#define IRQ12 44 /* PS/2 Mouse */
#define IRQ13 45 /* FPU */
#define IRQ14 46 /* Primary ATA */
#define IRQ15 47 /* Secondary ATA */

/* Syscall interrupt */
#define ISR_SYSCALL 0x80

/* Functions */
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);
void isr_register_handler(uint8_t num, isr_handler_t handler);

/* Assembly ISR stubs (defined in isr.asm) */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

/* IRQ stubs */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* Syscall stub */
extern void isr128(void);

#endif /* _IDT_H */
