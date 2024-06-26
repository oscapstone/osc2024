#include "../include/arm/sysregs.h"
#include "../include/entry.h"
#include "../include/sys.h"

.macro save_all, el
    sub sp, sp, #S_FRAME_SIZE
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

    .if \el == 0
        mrs x21, SP_EL0
    .else
        add x21, sp, #S_FRAME_SIZE
    .endif /* \el == 0 */

    stp x30, x21, [sp, 16 * 15]

    // push information for nested interrupt
    mrs x22, spsr_el1
    mrs x23, elr_el1
    stp x22, x23, [sp, 16 * 16]
.endm

.macro load_all, el
    // pop information for nested interrupt
    ldp x22, x23, [sp, 16 * 16]
    ldp x30, x21, [sp, 16 * 15]

    .if \el == 0
        msr SP_EL0, x21
    .endif /* \el == 0 */

    msr spsr_el1, x22
    msr elr_el1, x23

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

    add sp, sp, #S_FRAME_SIZE
    eret
.endm

.macro ventry label
    .align 7
    b      \label        
.endm

.macro handle_invalid_entry el, type
    save_all \el
    mov x0, #\type
    mrs x1, spsr_el1
    mrs x2, esr_el1
    mrs x3, elr_el1
    bl show_exception_info
    b err_hang
.endm

.macro handle_irq el
    save_all \el
    bl exception_el1_irq_handler
    load_all \el
.endm

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
ventry sync_invalid_el1t
ventry irq_invalid_el1t
ventry fiq_invalid_el1t
ventry error_invalid_el1t


ventry sync_invalid_el1h
ventry irq_invalid_el1h
ventry fiq_invalid_el1h
ventry error_invalid_el1h

ventry sync_invalid_el0_64
ventry irq_invalid_el0_64
ventry fiq_invalid_el0_64
ventry error_invalid_el0_64

ventry sync_invalid_el0_32
ventry irq_invalid_el0_32
ventry fiq_invalid_el0_32
ventry error_invalid_el0_32



sync_invalid_el1t:
handle_invalid_entry 1, SYNC_INVALID_EL1t

irq_invalid_el1t:
handle_invalid_entry 1, IRQ_INVALID_EL1t

fiq_invalid_el1t:
handle_invalid_entry 1, FIQ_INVALID_EL1t

error_invalid_el1t:
handle_invalid_entry 1, ERROR_INVALID_EL1t



sync_invalid_el1h:
handle_invalid_entry 1, SYNC_INVALID_EL1h

irq_invalid_el1h:
b el1_irq

fiq_invalid_el1h:
handle_invalid_entry 1, FIQ_INVALID_EL1h

error_invalid_el1h:
handle_invalid_entry 1, ERROR_INVALID_EL1h



sync_invalid_el0_64:
b el0_sync

irq_invalid_el0_64:
b el0_irq

fiq_invalid_el0_64:
handle_invalid_entry 0, FIQ_INVALID_EL0_64

error_invalid_el0_64:
handle_invalid_entry 0, ERROR_INVALID_EL0_64



sync_invalid_el0_32:
handle_invalid_entry 0, SYNC_INVALID_EL0_32

irq_invalid_el0_32:
handle_invalid_entry 0, IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
handle_invalid_entry 0, FIQ_INVALID_EL0_32

error_invalid_el0_32:
handle_invalid_entry 0, ERROR_INVALID_EL0_32

el1_irq:
    handle_irq 1

el0_irq:
    handle_irq 0

el0_sync:
    save_all 0
    mrs x25, esr_el1                        // read the syndrome register
    lsr x24, x25, #ESR_ELx_EC_SHIFT         // exception class
    cmp x24, #ESR_ELx_EC_SVC64              // SVC in 64-bit state
    b.eq el0_svc
    handle_invalid_entry 0 SYNC_ERROR

scnr .req x25
scno .req x26
stbl .req x27

el0_svc:
    adr stbl, sys_call_table
    uxtw scno, w8
    mov scnr, #__NR_syscalls
    bl enable_interrupt
    cmp scno, scnr
    b.hs ni_sys

    ldr x16, [stbl, scno, lsl #3]
    blr x16
    b ret_from_syscall

ni_sys:
    handle_invalid_entry 0, SYSCALL_ERROR

ret_from_syscall:
    bl disable_interrupt
    str x0, [sp, #S_X0]
    load_all 0

.globl ret_from_fork
ret_from_fork:
	bl	schedule_tail
	mov	x0, x20
	blr	x19 		//should never return

ret_to_user:
    bl disable_interrupt
    load_all 0

.globl err_hang
err_hang:
  b err_hang