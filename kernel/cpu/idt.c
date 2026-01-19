/*
 * NanoSec OS - Interrupt Descriptor Table
 * =========================================
 * IDT setup and interrupt dispatcher
 */

#include "idt.h"
#include "../kernel.h"


/* IDT with 256 entries */
static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

/* Interrupt handlers */
static isr_handler_t isr_handlers[256];

/* Exception messages */
static const char *exception_messages[] = {"Division By Zero",
                                           "Debug",
                                           "Non Maskable Interrupt",
                                           "Breakpoint",
                                           "Overflow",
                                           "Bound Range Exceeded",
                                           "Invalid Opcode",
                                           "Device Not Available",
                                           "Double Fault",
                                           "Coprocessor Segment Overrun",
                                           "Invalid TSS",
                                           "Segment Not Present",
                                           "Stack-Segment Fault",
                                           "General Protection Fault",
                                           "Page Fault",
                                           "Reserved",
                                           "x87 FPU Error",
                                           "Alignment Check",
                                           "Machine Check",
                                           "SIMD Floating-Point Exception",
                                           "Virtualization Exception",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved",
                                           "Reserved"};

/*
 * Set an IDT gate
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector,
                  uint8_t flags) {
  idt[num].base_low = base & 0xFFFF;
  idt[num].base_high = (base >> 16) & 0xFFFF;
  idt[num].selector = selector;
  idt[num].zero = 0;
  idt[num].flags = flags;
}

/*
 * Register an interrupt handler
 */
void isr_register_handler(uint8_t num, isr_handler_t handler) {
  isr_handlers[num] = handler;
}

/*
 * PIC ports
 */
#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1

/*
 * Remap PIC to avoid conflicts with CPU exceptions
 * IRQ 0-7  -> INT 32-39
 * IRQ 8-15 -> INT 40-47
 */
static void pic_remap(void) {
  uint8_t mask1, mask2;

  /* Save masks */
  mask1 = inb(PIC1_DATA);
  mask2 = inb(PIC2_DATA);

  /* Start initialization sequence */
  outb(PIC1_CMD, 0x11);
  io_wait();
  outb(PIC2_CMD, 0x11);
  io_wait();

  /* Set vector offsets */
  outb(PIC1_DATA, 0x20); /* IRQ 0-7 -> INT 32-39 */
  io_wait();
  outb(PIC2_DATA, 0x28); /* IRQ 8-15 -> INT 40-47 */
  io_wait();

  /* Tell Master PIC about Slave at IRQ2 */
  outb(PIC1_DATA, 0x04);
  io_wait();
  outb(PIC2_DATA, 0x02);
  io_wait();

  /* 8086 mode */
  outb(PIC1_DATA, 0x01);
  io_wait();
  outb(PIC2_DATA, 0x01);
  io_wait();

  /* Restore masks (enable all for now) */
  outb(PIC1_DATA, 0x00); /* Enable all IRQs */
  outb(PIC2_DATA, 0x00);
}

/*
 * Send End Of Interrupt to PIC
 */
void pic_eoi(uint8_t irq) {
  if (irq >= 8) {
    outb(PIC2_CMD, 0x20);
  }
  outb(PIC1_CMD, 0x20);
}

/*
 * Initialize the IDT
 */
void idt_init(void) {
  /* Clear handlers */
  for (int i = 0; i < 256; i++) {
    isr_handlers[i] = 0;
  }

  /* Set up IDTR */
  idt_ptr.limit = sizeof(idt) - 1;
  idt_ptr.base = (uint32_t)&idt;

  /* Clear IDT */
  memset(&idt, 0, sizeof(idt));

  /* Remap PIC */
  pic_remap();

  /* Install CPU exception handlers (ISR 0-31) */
  idt_set_gate(0, (uint32_t)isr0, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(1, (uint32_t)isr1, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(2, (uint32_t)isr2, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(3, (uint32_t)isr3, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(4, (uint32_t)isr4, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(5, (uint32_t)isr5, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(6, (uint32_t)isr6, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(7, (uint32_t)isr7, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(8, (uint32_t)isr8, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(9, (uint32_t)isr9, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(20, (uint32_t)isr20, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(21, (uint32_t)isr21, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(22, (uint32_t)isr22, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(23, (uint32_t)isr23, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(24, (uint32_t)isr24, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(25, (uint32_t)isr25, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(26, (uint32_t)isr26, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(27, (uint32_t)isr27, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(28, (uint32_t)isr28, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(29, (uint32_t)isr29, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(30, (uint32_t)isr30, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(31, (uint32_t)isr31, 0x08, IDT_FLAGS_KERNEL);

  /* Install IRQ handlers (32-47) */
  idt_set_gate(32, (uint32_t)irq0, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(33, (uint32_t)irq1, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(34, (uint32_t)irq2, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(35, (uint32_t)irq3, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(36, (uint32_t)irq4, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(37, (uint32_t)irq5, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(38, (uint32_t)irq6, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(39, (uint32_t)irq7, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(40, (uint32_t)irq8, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(41, (uint32_t)irq9, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_FLAGS_KERNEL);
  idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_FLAGS_KERNEL);

  /* Syscall interrupt (ring 3 accessible) */
  idt_set_gate(0x80, (uint32_t)isr128, 0x08, IDT_FLAGS_USER);

  /* Load IDT */
  asm volatile("lidt %0" : : "m"(idt_ptr));

  kprintf("  [OK] IDT (256 entries)\n");
}

/*
 * Common interrupt handler (called from assembly)
 */
void isr_handler(interrupt_frame_t *frame) {
  /* Check if we have a handler */
  if (isr_handlers[frame->int_no]) {
    isr_handlers[frame->int_no](frame);
  } else if (frame->int_no < 32) {
    /* Unhandled CPU exception */
    kprintf_color("\n!!! EXCEPTION: ", VGA_COLOR_RED);
    kprintf("%s (int %d, err %d)\n", exception_messages[frame->int_no],
            frame->int_no, frame->err_code);
    kprintf("EIP: 0x%x  CS: 0x%x\n", frame->eip, frame->cs);
    kprintf("EFLAGS: 0x%x\n", frame->eflags);

    /* Halt on unhandled exception */
    kprintf_color("System halted.\n", VGA_COLOR_RED);
    asm volatile("cli; hlt");
  }
}

/*
 * Common IRQ handler (called from assembly)
 */
void irq_handler(interrupt_frame_t *frame) {
  /* Send EOI */
  if (frame->int_no >= 40) {
    outb(PIC2_CMD, 0x20);
  }
  outb(PIC1_CMD, 0x20);

  /* Call handler if registered */
  if (isr_handlers[frame->int_no]) {
    isr_handlers[frame->int_no](frame);
  }
}
