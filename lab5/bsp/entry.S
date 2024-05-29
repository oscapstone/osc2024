#include <kernel/bsp_port/entry.h>
#include <kernel/bsp_port/syscall.h>
#include <bsp/asm/sysregs.h>

.global set_exception_vector_table

.macro handle_invalid_entry el, type
    kernel_entry \el
    mov    x0, #\type
    mrs    x1, esr_el1
    mrs    x2, elr_el1
    bl     show_invalid_entry_message
    b      err_hang
.endm

.macro kernel_entry, el
    sub    sp, sp, #S_FRAME_SIZE
    stp    x0, x1, [sp, #16 * 0]
    stp    x2, x3, [sp, #16 * 1]
    stp    x4, x5, [sp, #16 * 2]
    stp    x6, x7, [sp, #16 * 3]
    stp    x8, x9, [sp, #16 * 4]
    stp    x10, x11, [sp, #16 * 5]
    stp    x12, x13, [sp, #16 * 6]
    stp    x14, x15, [sp, #16 * 7]
    stp    x16, x17, [sp, #16 * 8]
    stp    x18, x19, [sp, #16 * 9]
    stp    x20, x21, [sp, #16 * 10]
    stp    x22, x23, [sp, #16 * 11]
    stp    x24, x25, [sp, #16 * 12]
    stp    x26, x27, [sp, #16 * 13]
    stp    x28, x29, [sp, #16 * 14]

    .if \el == 0
        mrs x21, sp_el0
    .else
        add x21, sp, #S_FRAME_SIZE
    .endif                     // \el == 0

    mrs x22, elr_el1
    mrs x23, spsr_el1

    stp x30, x21, [sp, #16 * 15]
    stp x22, x23, [sp, #16 * 16]
.endm

.macro kernel_exit, el
    ldp    x22, x23, [sp, #16 * 16]
    ldp    x30, x21, [sp, #16 * 15]

    .if \el == 0
        msr sp_el0, x21
    .endif          // \el == 0

    msr elr_el1, x22
    msr spsr_el1, x23

    ldp    x0, x1, [sp, #16 * 0]
    ldp    x2, x3, [sp, #16 * 1]
    ldp    x4, x5, [sp, #16 * 2]
    ldp    x6, x7, [sp, #16 * 3]
    ldp    x8, x9, [sp, #16 * 4]
    ldp    x10, x11, [sp, #16 * 5]
    ldp    x12, x13, [sp, #16 * 6]
    ldp    x14, x15, [sp, #16 * 7]
    ldp    x16, x17, [sp, #16 * 8]
    ldp    x18, x19, [sp, #16 * 9]
    ldp    x20, x21, [sp, #16 * 10]
    ldp    x22, x23, [sp, #16 * 11]
    ldp    x24, x25, [sp, #16 * 12]
    ldp    x26, x27, [sp, #16 * 13]
    ldp    x28, x29, [sp, #16 * 14]
    add    sp, sp, #S_FRAME_SIZE
    eret
.endm

.macro ventry label
    b      \label                   // branch to a handler function.
    .align 7                        // entry size is 0x80, .align will pad 0
.endm


/*
 * Exception vectors.
 */
.align	11
.globl vectors 
vectors:
	ventry	sync_invalid_el1t			// Synchronous EL1t
	ventry	irq_invalid_el1t			// IRQ EL1t
	ventry	fiq_invalid_el1t			// FIQ EL1t
	ventry	error_invalid_el1t			// Error EL1t

	ventry	sync_invalid_el1h			// Synchronous EL1h
    ventry	el1_irq				    	// IRQ EL1h
	ventry	fiq_invalid_el1h			// FIQ EL1h
	ventry	error_invalid_el1h			// Error EL1h

	ventry	el0_sync			    	// Synchronous 64-bit EL0
	ventry	el0_irq					    // IRQ 64-bit EL0
	ventry	fiq_invalid_el0_64			// FIQ 64-bit EL0
	ventry	error_invalid_el0_64		// Error 64-bit EL0

	ventry	sync_invalid_el0_32			// Synchronous 32-bit EL0
	ventry	irq_invalid_el0_32			// IRQ 32-bit EL0
	ventry	fiq_invalid_el0_32			// FIQ 32-bit EL0
	ventry	error_invalid_el0_32		// Error 32-bit EL0

sync_invalid_el1t:
	handle_invalid_entry 1,  SYNC_INVALID_EL1t

irq_invalid_el1t:
	handle_invalid_entry  1, IRQ_INVALID_EL1t

fiq_invalid_el1t:
	handle_invalid_entry  1, FIQ_INVALID_EL1t

error_invalid_el1t:
	handle_invalid_entry  1, ERROR_INVALID_EL1t

sync_invalid_el1h:
	handle_invalid_entry  1, SYNC_INVALID_EL1h

fiq_invalid_el1h:
	handle_invalid_entry  1, FIQ_INVALID_EL1h

error_invalid_el1h:
	handle_invalid_entry  1, ERROR_INVALID_EL1h

fiq_invalid_el0_64:
	handle_invalid_entry  0, FIQ_INVALID_EL0_64

error_invalid_el0_64:
	handle_invalid_entry  0, ERROR_INVALID_EL0_64

sync_invalid_el0_32:
	handle_invalid_entry  0, SYNC_INVALID_EL0_32

irq_invalid_el0_32:
	handle_invalid_entry  0, IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
	handle_invalid_entry  0, FIQ_INVALID_EL0_32

error_invalid_el0_32:
	handle_invalid_entry  0, ERROR_INVALID_EL0_32

// for start.S to set the exception vector table
set_exception_vector_table:
	adr x0, vectors                // get the base address of the exception vector table
	msr vbar_el1, x0               // set the base address of the exception vector table
	ret

el1_irq:
	kernel_entry 1 
	bl	el1_irq_entry
	kernel_exit 1 

el0_irq:
	kernel_entry 0 
	bl	el0_irq_entry
	kernel_exit 0 

el0_sync:
	kernel_entry 0
	mrs	x25, esr_el1				    // read the syndrome register
	lsr	x24, x25, #ESR_ELx_EC_SHIFT		// exception class
	cmp	x24, #ESR_ELx_EC_SVC64			// SVC in 64-bit state
	b.eq	el0_svc
	handle_invalid_entry 0, SYNC_ERROR

sc_nr	.req	x25  					// number of system calls
scno	.req	x26	   				    // syscall number
stbl	.req	x27					    // syscall table pointer

el0_svc:
	adr	stbl, sys_call_table			// load syscall table pointer
	uxtw	scno, w8				    // syscall number in w8
	mov	sc_nr, #__NR_syscalls
	bl	enable_irq
	cmp     scno, sc_nr               	// check upper syscall limit
	b.hs	ni_sys

	ldr	x16, [stbl, scno, lsl #3]		// address in the syscall table
	blr	x16					            // call sys_* routine
	b	ret_from_syscall
ni_sys:
	handle_invalid_entry 0, SYSCALL_ERROR
ret_from_syscall:
	bl	disable_irq				
	str	x0, [sp, #S_X0]				    // returned x0
	kernel_exit 0


// page 2107
.global from_el2_to_el1
from_el2_to_el1:
	mov x0, (1 << 31) // EL1 uses aarch64
	msr hcr_el2, x0   // msr: Status Register <-- Register (page 1924)
	mov x0, 0x3c5     // EL1h (SPSel = 1) with interrupt disabled (D = 1, A = 1, I = 1, F = 1, M[3:0] = 5) EL1h
	msr spsr_el2, x0  // Save the current processor's state
	msr elr_el2, lr   // Save the exception return address
	eret              // return to EL1 (use eret to return from the exception)

.global from_el1_to_el0
from_el1_to_el0:
	mov x2, 0x3c0
	msr spsr_el1, x2
	msr sp_el0, x1                      // pass the stack pointer address from arg
	msr elr_el1, x0                     // pass the program's start address from arg
	eret

.globl ret_from_fork
ret_from_fork:
	bl	schedule_tail
	cbz	x19, ret_to_user			    // not a kernel thread
	mov	x0, x20                         // restore arguments
	blr	x19                             // return to the new process
ret_to_user:
	bl disable_irq				
    // bl sys_debug_msg
	kernel_exit 0 

err_hang:
    b     err_hang
