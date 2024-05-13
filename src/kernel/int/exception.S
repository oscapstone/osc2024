.section .text
.global set_exception_vector_table
set_exception_vector_table:
    adr     x0, exception_vector_table
    // D8.2.96 VBAR_EL1, Vector Base Address Register (EL1)
    msr     VBAR_EL1, x0
    ret

// save general registers to stack
.macro save_all el
    sub     sp, sp, 16 * 17
    stp     x0, x1, [sp ,16 * 0]
    stp     x2, x3, [sp ,16 * 1]
    stp     x4, x5, [sp ,16 * 2]
    stp     x6, x7, [sp ,16 * 3]
    stp     x8, x9, [sp ,16 * 4]
    stp     x10, x11, [sp ,16 * 5]
    stp     x12, x13, [sp ,16 * 6]
    stp     x14, x15, [sp ,16 * 7]
    stp     x16, x17, [sp ,16 * 8]
    stp     x18, x19, [sp ,16 * 9]
    stp     x20, x21, [sp ,16 * 10]
    stp     x22, x23, [sp ,16 * 11]
    stp     x24, x25, [sp ,16 * 12]
    stp     x26, x27, [sp ,16 * 13]
    stp     x28, x29, [sp ,16 * 14]
    mrs     x0, SPSR_EL1
    stp     lr, x0, [sp, 16 * 15]
    mrs     x1, ELR_EL1
    .if \el == 0
    mrs     x2, SP_EL0
    .else
    mov     x2, xzr
    .endif
    stp     x1, x2, [sp, 16 * 16]
.endm

// load general registers from stack
.macro load_all el
    ldp     x1, x2, [sp, 16 * 16]
    .if \el == 0
    msr     SP_EL0, x2
    .endif
    msr     ELR_EL1, x1
    ldp     lr, x0, [sp, 16 * 15]
    msr     SPSR_EL1, x0
    ldp     x28, x29, [sp ,16 * 14]
    ldp     x26, x27, [sp ,16 * 13]
    ldp     x24, x25, [sp ,16 * 12]
    ldp     x22, x23, [sp ,16 * 11]
    ldp     x20, x21, [sp ,16 * 10]
    ldp     x18, x19, [sp ,16 * 9]
    ldp     x16, x17, [sp ,16 * 8]
    ldp     x14, x15, [sp ,16 * 7]
    ldp     x12, x13, [sp ,16 * 6]
    ldp     x10, x11, [sp ,16 * 5]
    ldp     x8, x9, [sp ,16 * 4]
    ldp     x6, x7, [sp ,16 * 3]
    ldp     x4, x5, [sp ,16 * 2]
    ldp     x2, x3, [sp ,16 * 1]
    ldp     x0, x1, [sp ,16 * 0]
    add     sp, sp, 16 * 17
.endm

.macro CALL_WITH_CONTEXT type handler
    .if \type < 8
    save_all 1
    .else
    save_all 0
    .endif
    mov     x1, \type
    mov     x0, sp
    bl      \handler
    .if \type >= 8
    mov     x0, sp
    bl      return_to_user
    .endif
    .if \type < 8
    b       exception_restore_context_EL1
    .else
    b       exception_restore_context_EL0
    .endif
.endm

// D1.10.2 Exception vectors
// https://developer.arm.com/documentation/102412/0103/Handling-exceptions/Taking-an-exception
.section .text.exception_vector
.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    .org 0x000
    CALL_WITH_CONTEXT 0 print_exception
    .org 0x080
    CALL_WITH_CONTEXT 1 print_exception
    .org 0x100
    CALL_WITH_CONTEXT 2 print_exception
    .org 0x180
    CALL_WITH_CONTEXT 3 print_exception

    .org 0x200
    CALL_WITH_CONTEXT 4 sync_handler
    .org 0x280
    CALL_WITH_CONTEXT 5 irq_handler
    .org 0x300
    CALL_WITH_CONTEXT 6 print_exception
    .org 0x380
    CALL_WITH_CONTEXT 7 print_exception

    .org 0x400
    CALL_WITH_CONTEXT 8 sync_handler
    .org 0x480
    CALL_WITH_CONTEXT 9 irq_handler
    .org 0x500
    CALL_WITH_CONTEXT 10 print_exception
    .org 0x580
    CALL_WITH_CONTEXT 11 print_exception

    .org 0x600
    CALL_WITH_CONTEXT 12 print_exception
    .org 0x680
    CALL_WITH_CONTEXT 13 print_exception
    .org 0x700
    CALL_WITH_CONTEXT 14 print_exception
    .org 0x780
    CALL_WITH_CONTEXT 15 print_exception
    .org 0x800

exception_restore_context_EL0:
    load_all 0
    eret

exception_restore_context_EL1:
    load_all 1
    eret
