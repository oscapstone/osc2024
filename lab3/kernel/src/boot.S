.section ".text.boot"

.global _start

_start:
    bl from_el2_to_el1

set_exception_vector_table:
    adr x0, exception_vector_table  // vbar_el1: EL1 Vector Base Address Register
    msr vbar_el1, x0

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
    bl      kernel_main

proc_hang:
    wfe
    b       proc_hang

from_el2_to_el1:
    mov x1, (1 << 31)              // set HCR_EL2（Hypervisor Configuration Register EL2）
    msr hcr_el2, x1                //           RW[31]: 0b1 The processor execution environment for EL1 is AArch64
    mov x1, 0x3c5                  // spsr_el2: Holds the saved process state when an exception is taken to EL2.
    msr spsr_el2, x1               //           (1)[1111] 00 (2)[0101] -> (1) EL2-PSTATE.DAIF Disabled (2) Exception level = EL1h
    msr elr_el2, lr                // lr (stored return address) -> elr_el2
    eret                           // Perform an exception return. EL2 -> EL1