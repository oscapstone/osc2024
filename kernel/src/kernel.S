.section ".text._start_kernel"

.global _start_kernel

_start_kernel:
    mrs x0, CurrentEL
    and x0, x0, #0xc
    cmp x0, #0b1000
    bne _loop

    bl from_el2_to_el1

    adr x0, exception_vector_table
    msr vbar_el1, x0

    b _start_rust

from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    mov x0, sp
    msr sp_el1, x0
    eret // return to EL1

_loop:
    b _loop


.macro EXCEPTION_WITH_TYPE type handler
    save_all
    mov x0, \type
    bl \handler
    b exception_restore_context
.endm

exception_restore_context:
    load_all
    eret


.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    .align 7
    EXCEPTION_WITH_TYPE 0, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 1, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 2, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 3, exception_handler
    
    .align 7
    EXCEPTION_WITH_TYPE 4, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 5, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 6, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 7, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 8, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 9, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 10, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 11, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 12, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 13, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 14, exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 15, exception_handler


msr DAIFClr, #0xf
