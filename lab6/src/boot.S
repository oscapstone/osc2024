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

    ldr x1, =__PA_stack_end
    mov sp, x1
    mov x19, x0
    bl mmu_init
    mov x0, x19
    ldr x1, =boot_rest
    br x1

boot_rest:

set_exception_vector:
    // set exception vector table
    adr x20, exception_vector_table
    msr vbar_el1, x20 // vector base address register

set_stack:
	ldr x1, =__stack_end // __stack_end = 0x0200000
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


// save general registers to stack
.macro save_all
    sub sp, sp, 32*9
    stp x0, x1, [sp, 16*0]
    stp x2, x3, [sp, 16*1]
    stp x4, x5, [sp, 16*2]
    stp x6, x7, [sp, 16*3]
    stp x8, x9, [sp, 16*4]
    stp x10, x11, [sp, 16*5]
    stp x12, x13, [sp, 16*6]
    stp x14, x15, [sp, 16*7]
    stp x16, x17, [sp, 16*8]
    stp x18, x19, [sp, 16*9]
    stp x20, x21, [sp, 16*10]
    stp x22, x23, [sp, 16*11]
    stp x24, x25, [sp, 16*12]
    stp x26, x27, [sp, 16*13]
    stp x28, x29, [sp, 16*14]
    mrs x0, spsr_el1
    stp x30, x0, [sp, 16*15]
    mrs x0, elr_el1
    mrs x1, sp_el0
    stp x0, x1, [sp, 16*16]
.endm

// load general registers from stack
.macro load_all
    ldp x0, x1, [sp, 16*16]
    msr elr_el1, x0
    msr sp_el0, x1
    ldp x30, x0, [sp, 16*15]
    msr spsr_el1, x0
    ldp x0, x1, [sp, 16*0]
    ldp x2, x3, [sp, 16*1]
    ldp x4, x5, [sp, 16*2]
    ldp x6, x7, [sp, 16*3]
    ldp x8, x9, [sp, 16*4]
    ldp x10, x11, [sp, 16*5]
    ldp x12, x13, [sp, 16*6]
    ldp x14, x15, [sp, 16*7]
    ldp x16, x17, [sp, 16*8]
    ldp x18, x19, [sp, 16*9]
    ldp x20, x21, [sp, 16*10]
    ldp x22, x23, [sp, 16*11]
    ldp x24, x25, [sp, 16*12]
    ldp x26, x27, [sp, 16*13]
    ldp x28, x29, [sp, 16*14]
    add sp, sp, 32*9
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

user_exception_handler:
    save_all
    mov x0, sp // trapframe store in the top of kernel stack
    bl user_exception_handler_c
    load_all
    eret

user_irq_exception_handler:
    save_all
    bl user_irq_exception_handler_c
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

    b user_exception_handler
    .align 7
    b user_irq_exception_handler
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

