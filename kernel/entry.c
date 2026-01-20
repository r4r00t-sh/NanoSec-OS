/*
 * Kernel Entry - Must be at start of kernel binary
 */

/* Entry point - jumps to kernel_main */
asm(".code32\n"
    ".global _start\n"
    "_start:\n"
    "    call kernel_main\n"
    "    hlt\n");
