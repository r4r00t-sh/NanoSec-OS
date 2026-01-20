/* NanoSec OS - Multiboot Linker Script */

ENTRY(_start)

SECTIONS
{
    /* Start at 1MB - standard for multiboot kernels */
    . = 1M;
    
    /* Multiboot header must be first */
    .multiboot ALIGN(4K) :
    {
        *(.multiboot)
    }
    
    /* Text section */
    .text ALIGN(4K) :
    {
        *(.text)
    }
    
    /* Read-only data */
    .rodata ALIGN(4K) :
    {
        *(.rodata)
    }
    
    /* Initialized data */
    .data ALIGN(4K) :
    {
        *(.data)
    }
    
    /* Uninitialized data (BSS) */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
    }
    
    /* Kernel end marker */
    _kernel_end = .;
}
