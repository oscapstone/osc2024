#include "mm.h"
#include "exception.h"

.section ".text.boot"

.globl _start
_start:
    mov x25, x0
	mrs	x0, mpidr_el1
	and x0, x0, #0xFF
	cbz	x0, master
	b 	proc_hang

proc_hang:
	wfe
	b 	proc_hang

master:
    bl  check_cur_el2
	bl 	from_el2_to_el1
    bl  check_cur_el1
	bl	set_exception_table
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl	memzero

	mov	sp, #LOW_MEMORY
	bl	kernel_main

check_cur_el2:
    mrs x0, CurrentEL
    and x0, x0, 12  // take CurrentEL[3:2]
    lsr x0, x0, 2   // logical shift right 
    sub x0, x0, 2
    cbnz x0, proc_hang
    ret

check_cur_el1:
    mrs x0, CurrentEL
    and x0, x0, 12  // take CurrentEL[3:2]
    lsr x0, x0, 2   // logical shift right 
    sub x0, x0, 1
    cbnz x0, proc_hang
    ret

/* Rpi3’s CPU runs in EL2 after being booted by default, but we want the kernel to run in EL1 */
/*
    SPSR -> D A I F bit[9-6]
    0x3c5   1 1 1 1   mask all interrupt
    0x340   1 1 0 1   unmask IRQ
*/
.globl from_el2_to_el1
from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // 11_1100_0101 disabled all interrupts from el2, EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0 // Saved Program Status Registers
    msr elr_el2, lr // link register(return address)
    eret // return to EL1

/* After the kernel is initialized, load user programs and execute them in EL0 by eret. */
.globl from_el1_to_el0 // (prog_addr, usr_prog stack_top) 
from_el1_to_el0:    
    msr elr_el1, x0 // set to prog start addr
    msr sp_el0, x1 // set user prog sp to a proper pos by setting sp_el0
    mov x1, 0x340  // 11_0100_0000 only enabled IRQ (timer & uart) interrupts from el1
    msr spsr_el1, x1
    eret

// save all general registers to stack
.macro save_all
    sub sp, sp, 32 * 8
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
    add sp, sp, 32 * 8
.endm

exception_handler:
    save_all
    bl undefined
    load_all
    eret

sync_handler:
    save_all
    bl sync
    load_all
    eret

irq_handler_from_el0:
    save_all
    bl irq_from_el0
    load_all
    eret

irq_handler_from_el1:
    save_all
    bl irq_from_el1
    load_all
    eret

/**
< Exception Vector >
total 16 exception handler, 4 exception levels x 4 execution states
exception levels: Synchronous exception, IRQ, FIQ, SError
execution states: EL1t, EL1h(currently using), EL064(currently using), EL032

each entry should be located at offset 0x80 bytes, by .align 7 we get 2^7 alignment

*/ 

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
  	b 	exception_handler // branch to a handler function. // Sync EL1t
  	.align 7
	b 	exception_handler // IRQ EL1t
	.align 7
	b 	exception_handler // FIQ EL1t
	.align 7
	b 	exception_handler // SError EL1t
	.align 7

	b 	exception_handler // VBAR_ELn + 0x200 // Sync EL1h
	.align 7
	b 	irq_handler_from_el1 // **IRQ EL1h, kernel takes exceptions
	.align 7
	b 	exception_handler
	.align 7
	b 	exception_handler
	.align 7

	b 	sync_handler // VBAR_ELn + 0x400
	.align 7
	b 	irq_handler_from_el0 // 64 bit user program take exceptions
	.align 7
	b 	exception_handler
	.align 7
	b 	exception_handler
	.align 7

	b 	exception_handler
	.align 7
	b 	exception_handler
	.align 7
	b 	exception_handler
	.align 7
	b 	exception_handler
	.align 7

set_exception_table:
  	adr x0, exception_vector_table
  	msr vbar_el1, x0 // Vector Base Address Register, to tell processor where exception table is
    ret