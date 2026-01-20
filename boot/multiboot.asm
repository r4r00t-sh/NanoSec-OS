; NanoSec OS - Multiboot Header
; Allows GRUB to load our kernel with VESA framebuffer

[BITS 32]

; Multiboot constants
MULTIBOOT_MAGIC     equ 0x1BADB002
; Flags: Align modules (0), Memory info (1), Video mode (2)
MULTIBOOT_FLAGS     equ 0x00000007
MULTIBOOT_CHECKSUM  equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM
    ; Address fields (unused with ELF)
    dd 0    ; header_addr
    dd 0    ; load_addr
    dd 0    ; load_end_addr
    dd 0    ; bss_end_addr
    dd 0    ; entry_addr
    ; Video mode fields
    dd 0    ; mode_type (0 = linear graphics)
    dd 800  ; width
    dd 600  ; height
    dd 32   ; depth (bits per pixel)

section .bss
align 16
stack_bottom:
    resb 16384      ; 16KB stack
stack_top:

section .data
global multiboot_magic
global multiboot_info
multiboot_magic: dd 0
multiboot_info:  dd 0

section .text
global _start
extern kernel_main

_start:
    ; Save multiboot info
    mov [multiboot_magic], eax
    mov [multiboot_info], ebx
    
    ; Set up stack
    mov esp, stack_top
    
    ; Push multiboot info for kernel
    push ebx        ; Multiboot info pointer
    push eax        ; Multiboot magic
    
    ; Call kernel
    call kernel_main
    
    ; Hang if kernel returns
    cli
.hang:
    hlt
    jmp .hang
