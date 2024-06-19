#include "sysregs.h"
.section ".text.boot"

.global _start

_start:
    bl from_el2_to_el1

set_exception_vector_table:
    adr x1, exception_table // vbar_el1: Vector Base Address Register (EL1)
    msr vbar_el1, x1               //           Holds the exception base address for any exception that is taken to EL1.

setup_stack:
    ldr     x1, =__stack_top
    mov     sp, x1

setup_bss:
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

init_bss:
    cbz     w2, run_main
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, init_bss

run_main:
    bl      main

proc_hang:
    wfe
    b       proc_hang

from_el2_to_el1:
    mov x1, (1 << 31)              // hcr_el2: Execution state control for EL2
    msr hcr_el2, x1                //          RW[31]: 0b1 The processor execution environment for EL1 is AArch64
    msr sctlr_el1, xzr             // sctlr_el1: System Control Register (EL1) disables MMU
    mov x1, 0x3c5                  // spsr_el2: Holds the saved process state when an exception is taken to EL2.
    msr spsr_el2, x1               //           (1)[1111] 00 (2)[0101] -> (1) EL2-PSTATE.DAIF Disabled (2) Exception level = EL1h
    msr elr_el2, lr                // elr_el2: When taking an exception to EL2, holds the address to return to.
    eret                           // eret: Perform an exception return. EL2 -> EL1