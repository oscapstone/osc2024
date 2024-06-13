.section ".text.boot"

.global _start

_start:
    bl from_el2_to_el1

set_exception_vector_table:
    adr x1, exception_vector_table
    msr vbar_el1, x1

setting:
    ldr x1, =_stack_top
    // ldr x1, =_start
    mov sp, x1

    ldr x1, =_bss_top
    ldr w2, =_bss_size
    // ldr x1, =_bss_start
    // ldr w2, =_bss_size

clear_bss:
    cbz w2, run_main
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, clear_bss

run_main:
    ldr x1, =dtb_ptr
    str x0, [x1], #8
    bl main

hang:
    wfe
    b hang

from_el2_to_el1:
    mov x1, (1 << 31)              // hcr_el2: Execution state control for EL2
    msr hcr_el2, x1                //          RW[31]: 0b1 The processor execution environment for EL1 is AArch64
    mov x1, 0x3c5                  // spsr_el2: Holds the saved process state when an exception is taken to EL2.
    msr spsr_el2, x1               //           (1)[1111] 00 (2)[0101] -> (1) EL2-PSTATE.DAIF Disabled (2) Exception level = EL1h
    msr elr_el2, lr                // elr_el2: When taking an exception to EL2, holds the address to return to.
    eret                           // eret: Perform an exception return. EL2 -> EL1
