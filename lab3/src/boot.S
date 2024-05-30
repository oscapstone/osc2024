#include "mm.h"
#define CORE0_TIMER_IRQ_CTRL 0x40000040

.section ".text.boot"

.globl _start

_start:
	mrs	x1, mpidr_el1		
	and	x1, x1,#0xFF		// Check processor id
	cbz	x1, master		// Hang for all non-primary CPU

proc_hang:
	wfe
	b 	proc_hang

master:
    bl from_el2_to_el1

// core_timer_enable:
//    mov x20, 1
//    msr cntp_ctl_el0, x20 // enable timer
//    mrs x20, cntfrq_el0
//    msr cntp_tval_el0, x20 // set expired time
//    mov x20, 2
//    ldr x1, =CORE0_TIMER_IRQ_CTRL
//    str w20, [x1] // unmask timer interrupt

set_exception_vector:
    // set exception vector table
    adr x20, exception_vector_table
    msr vbar_el1, x20 // vector base address register

set_stack:
	ldr x1, =_start
    mov sp, x1
	ldr x1, =bss_begin
    ldr w2, =__bss_size

bss_reset:  
    cbz     w2, run_main
    // save 0 and plus 8
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, bss_reset

run_main:
	bl	main
	b 	proc_hang		// should never come here


from_el2_to_el1:
    mov x1, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x1
    // 0x3c5 = 0b1111000101
    //           9876543210
    //           DAIFRM
    // 0x345 = 0b1101000101
    //           9876543210
    //           DAIFRM
    // M[3:0] = 0101 = EL1h
    mov x1, 0x345 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x1
    msr elr_el2, lr
    eret // return to EL1



.macro save_all
    sub sp, sp, 32 * 9
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]

    // regiester for nested exception
    mrs x0, spsr_el1
    str x0, [sp, 16 * 16]
    mrs x0, elr_el1
    str x0, [sp, 16 * 17]
    ldp x0, x1, [sp ,16 * 0]  // restore x0 instead of elr_el1
.endm

// load general registers from stack
.macro load_all
    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    // restore nested exception
    ldr x0, [sp, 16 * 16]
    msr spsr_el1,x0
    ldr x0, [sp, 16 * 17]
    msr elr_el1, x0
    ldp x0, x1, [sp ,16 * 0]  // restore x0 instead of elr_el1

    add sp, sp, 32 * 9
.endm

exception_handler:
    save_all
    bl exception_handler_c
    load_all
    eret

irq_exception_handler:
    save_all
    bl irq_exception_handler_c
    load_all
    eret

// EL1 exception vector table
.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    b exception_handler // branch to a handler function.
    .align 7 // entry size is 0x80, .align will pad 0
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b exception_handler
    .align 7
    b irq_exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b exception_handler
    .align 7
    b irq_exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

