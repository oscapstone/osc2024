#include "arm/sysregs.h"
// LAB3-1
.globl el2_to_el1
el2_to_el1:
    // disable MMU
    ldr     x1, =SCTLR_VALUE_MMU_DISABLED
    msr     sctlr_el1, x1
    // make el0, el1 can use Floating point and Advanced SIMD
    ldr     x1, =CPACR_EL1_VALUE
    msr     CPACR_EL1, x1
    // set AArch64 for el2
    ldr     x1, =HCR_EL2_VALUE
    msr     hcr_el2, x1
    // mask all interrupt, and set interrupt level to el1h
    ldr     x1, =SPSR_EL2_VALUE
    msr     spsr_el2, x1
    msr     elr_el2, lr
    eret

// LAB3-2
.globl branch_el1_to_el0
branch_el1_to_el0:
    mov x2, 0x3c0
    // msr modify special register
    msr spsr_el1, x2
    // x0 is addr of user program
    msr elr_el1, x0
    // x1 is sp of user program
    msr sp_el0, x1
    eret
