/* ARMv8 Assembly Instruction */
/**

mov x0, x1
    sets: x0 = x1
ldr x0, <addr>
    load 32bits from <addr> to x0
ldr w0, <addr>
    load 64bits from <addr> to w0
cbz x0, <label>
    if x0 == 0, jump to <label>
cbnz x0, <label>
    if x0 != 0, jump to <label>
str x0 [x1] #8
    store x0 in addr<x1> then x1=x1+8
b   <label>
    jump to <label>
bl  <label>
    jump to <label> and copies bl's next instruction into link register
wfe
    Wait for event, core in low-power state (power on, clk off)

**/

.section ".text.boot"

.global _start

_start:
    bl from_el2_to_el1

set_exception_vector_table:
    adr x1, exception_vector_table // vbar_el1: Vector Base Address Register (EL1)
    msr vbar_el1, x1               //           Holds the exception base address for any exception that is taken to EL1.

setup_stack:
    ldr     x1, =_stack_top
    mov     sp, x1

setup_bss:
    ldr     x1, =_bss_top
    ldr     w2, =_bss_size

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