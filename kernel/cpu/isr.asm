; NanoSec OS - Interrupt Service Routines (Assembly)
; ====================================================
; Low-level ISR stubs that save state and call C handlers

[BITS 32]

; External C handlers
extern isr_handler
extern irq_handler

; Macro for ISR without error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0        ; Dummy error code
    push dword %1       ; Interrupt number
    jmp isr_common
%endmacro

; Macro for ISR with error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push dword %1       ; Interrupt number (error code already pushed)
    jmp isr_common
%endmacro

; Macro for IRQ
%macro IRQ 2
global irq%1
irq%1:
    push dword 0        ; Dummy error code
    push dword %2       ; Interrupt number
    jmp irq_common
%endmacro

; CPU Exceptions (ISR 0-31)
ISR_NOERRCODE 0     ; Divide by zero
ISR_NOERRCODE 1     ; Debug
ISR_NOERRCODE 2     ; NMI
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Overflow
ISR_NOERRCODE 5     ; Bound range
ISR_NOERRCODE 6     ; Invalid opcode
ISR_NOERRCODE 7     ; Device not available
ISR_ERRCODE   8     ; Double fault
ISR_NOERRCODE 9     ; Coprocessor segment overrun
ISR_ERRCODE   10    ; Invalid TSS
ISR_ERRCODE   11    ; Segment not present
ISR_ERRCODE   12    ; Stack fault
ISR_ERRCODE   13    ; General protection fault
ISR_ERRCODE   14    ; Page fault
ISR_NOERRCODE 15    ; Reserved
ISR_NOERRCODE 16    ; x87 FPU error
ISR_ERRCODE   17    ; Alignment check
ISR_NOERRCODE 18    ; Machine check
ISR_NOERRCODE 19    ; SIMD error
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; Hardware IRQs (mapped to INT 32-47)
IRQ 0, 32   ; Timer
IRQ 1, 33   ; Keyboard
IRQ 2, 34   ; Cascade
IRQ 3, 35   ; COM2
IRQ 4, 36   ; COM1
IRQ 5, 37   ; LPT2
IRQ 6, 38   ; Floppy
IRQ 7, 39   ; LPT1
IRQ 8, 40   ; RTC
IRQ 9, 41   ; Free
IRQ 10, 42  ; Free
IRQ 11, 43  ; Free
IRQ 12, 44  ; PS/2 Mouse
IRQ 13, 45  ; FPU
IRQ 14, 46  ; Primary ATA
IRQ 15, 47  ; Secondary ATA

; Syscall (INT 0x80)
global isr128
isr128:
    push dword 0
    push dword 128
    jmp isr_common

; Common ISR stub - saves state and calls C handler
isr_common:
    ; Save all registers
    pusha
    
    ; Save data segment
    mov ax, ds
    push eax
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push pointer to stack frame
    push esp
    
    ; Call C handler
    call isr_handler
    
    ; Pop stack frame pointer
    add esp, 4
    
    ; Restore data segment
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Restore registers
    popa
    
    ; Clean up interrupt number and error code
    add esp, 8
    
    ; Return from interrupt
    iret

; Common IRQ stub
irq_common:
    pusha
    
    mov ax, ds
    push eax
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp
    call irq_handler
    add esp, 4
    
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa
    add esp, 8
    iret
