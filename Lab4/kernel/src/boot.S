#include "mm.h"
#include "arm/sysregs.h"

.section ".text.boot"

.globl _start
_start:
    mov x10, x0 // protect the dtb address sent from the bootloader

    mrs x1, mpidr_el1
    and x1, x1, #3
    cbz x1, master
    b proc_hang

proc_hang:
    b proc_hang

/* Master core */
master:
    mrs x1, CurrentEL
    lsr x1, x1, #2

    // running at EL3?
    cmp x1, #3
    bne from_el2_to_el1

    ldr x1, =SCTLR_VALUE
    msr sctlr_el1, x1

    ldr x1, =HCR_VALUE
    msr hcr_el2, x1

    ldr x1, =SCR_VALUE
    msr scr_el3, x1

    ldr x1, =SPSR_VALUE
    msr spsr_el3, x1

    adr x1, el1_entry
    msr elr_el3, x1

    eret

from_el2_to_el1:
    ldr x1, =SCTLR_VALUE
    msr sctlr_el1, x1

    ldr x1, =HCR_VALUE
    msr hcr_el2, x1

    ldr x1, =SPSR_VALUE
    msr spsr_el2, x1

    adr x1, el1_entry
    msr elr_el2, x1

    eret

el1_entry:
    adr x0, bss_begin
    adr x1, bss_end
    sub x1, x1, x0
    bl memzero

    mov x0, x10
    mov sp, #LOW_MEMORY
    bl kernel_main
    b proc_hang     // should never come here
