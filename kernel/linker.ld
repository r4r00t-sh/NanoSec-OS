/*
 * Linker Script - Outputs flat binary at 0x10000
 */

OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    . = 0x10000;
    
    .text : {
        *(.text._start)
        *(.text)
    }
    
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
