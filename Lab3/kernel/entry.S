#include "entry.h"

.macro handle_invalid_entry type
mov x0, #\type
mrs x1, esr_el1
mrs x2, elr_el1
bl  show_invalid_entry_message
b err_hang
.endm

/*
 *  Vector table entries.
 *  All exception vectors should be located at offset 0x80 bytes one from another. 
 */
.macro  ventry  label
.align  7
b       \label
.endm

/*
 *  Save current register state before exception handling.
 */
.macro save_state
    //  32 GPR, each register is 8 bytes(64-bit)
    sub sp, sp, 32 * 8
    //  stp(Store Pair of registers)
    stp x0, x1, [sp, 16 * 0]
    stp x2, x3, [sp, 16 * 1]
    stp x4, x5, [sp, 16 * 2]
    stp x6, x7, [sp, 16 * 3]
    stp x8, x9, [sp, 16 * 4]
    stp x10, x11, [sp, 16 * 5]
    stp x12, x13, [sp, 16 * 6]
    stp x14, x15, [sp, 16 * 7]
    stp x16, x17, [sp, 16 * 8]
    stp x18, x19, [sp, 16 * 9]
    stp x20, x21, [sp, 16 * 10]
    stp x22, x23, [sp, 16 * 11]
    stp x24, x25, [sp, 16 * 12]
    stp x26, x27, [sp, 16 * 13]
    stp x28, x29, [sp, 16 * 14]
    str x30, [sp, 16 * 15]
.endm

/*
 *  Load current register state after exception handling.
 */
.macro load_state
    ldp x0, x1, [sp, 16 * 0]
    ldp x2, x3, [sp, 16 * 1]
    ldp x4, x5, [sp, 16 * 2]
    ldp x6, x7, [sp, 16 * 3]
    ldp x8, x9, [sp, 16 * 4]
    ldp x10, x11, [sp, 16 * 5]
    ldp x12, x13, [sp, 16 * 6]
    ldp x14, x15, [sp, 16 * 7]
    ldp x16, x17, [sp, 16 * 8]
    ldp x18, x19, [sp, 16 * 9]
    ldp x20, x21, [sp, 16 * 10]
    ldp x22, x23, [sp, 16 * 11]
    ldp x24, x25, [sp, 16 * 12]
    ldp x26, x27, [sp, 16 * 13]
    ldp x28, x29, [sp, 16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 32 * 8
.endm

/*
 *  Setup interrupt vector table.
 * Vector table should be aligned to 0x800.
 */
 .align     11
 .global    vectors
 vectors:
    /*
     *  EL1t Exception is taken from EL1 while stack pointer was shared with EL0.
     *  This happens when SPSel register holds the value 0. In our lab, we won't be using this
     *  exception. Including all 16 handlers for easier debugging if anything goes wrong.
     */
    ventry  sync_invalid_el1t           // Synchronous EL1t
    ventry  irq_invalid_el1t            // IRQ EL1t
    ventry	fiq_invalid_el1t			// FIQ EL1t
	ventry	error_invalid_el1t			// Error EL1t

    /*
     *  EL1h Exception is taken from EL1 at the time when dedicated stack pointer was allocated
     *  for EL1. This means that SPSel holds the value 1 and this is the mode that we are currently using.
     */
    ventry  sync_el1h                   // Synchronous EL1h
    ventry  irq_el1h                    // IRQ EL1h
    ventry  fiq_invalid_el1h            // FIQ EL1h
    ventry  error_invalid_el1h          // Error EL1h

    /*
     *  EL0_64 Exception is taken from EL0 executing in 64-bit mode.
     */
    ventry	sync_el0_64			        // Synchronous 64-bit EL0
	ventry	irq_el0_64			        // IRQ 64-bit EL0
	ventry	fiq_invalid_el0_64			// FIQ 64-bit EL0
	ventry	error_invalid_el0_64	    // Error 64-bit EL0

    /*
     *  EL0_32 Exception is taken from EL0 executing in 32-bit mode.(Won't be using it)
     */
    ventry	sync_invalid_el0_32			// Synchronous 32-bit EL0
	ventry	irq_invalid_el0_32			// IRQ 32-bit EL0
	ventry	fiq_invalid_el0_32			// FIQ 32-bit EL0
	ventry	error_invalid_el0_32		// Error 32-bit EL0

sync_invalid_el1t:
    handle_invalid_entry    SYNC_INVALID_EL1t

irq_invalid_el1t:
	handle_invalid_entry    IRQ_INVALID_EL1t

fiq_invalid_el1t:
	handle_invalid_entry    FIQ_INVALID_EL1t

error_invalid_el1t:
	handle_invalid_entry    ERROR_INVALID_EL1t

fiq_invalid_el1h:
    handle_invalid_entry    FIQ_INVALID_EL1h

error_invalid_el1h:
    handle_invalid_entry    ERROR_INVALID_EL1h

fiq_invalid_el0_64:
    handle_invalid_entry    FIQ_INVALID_EL0_64

error_invalid_el0_64:
    handle_invalid_entry    ERROR_INVALID_EL0_64

sync_invalid_el0_32:
    handle_invalid_entry    SYNC_INVALID_EL0_32

irq_invalid_el0_32:
    handle_invalid_entry    IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
    handle_invalid_entry    FIQ_INVALID_EL0_32

error_invalid_el0_32:
    handle_invalid_entry    ERROR_INVALID_EL0_32

// Jump to synchronous exception handler.
sync_el0_64:
    save_state
    bl handle_sync_el0_64
    load_state
    eret

// Jump to interrupt exception handler.
irq_el0_64:
    save_state
    bl handle_irq_el0_64
    load_state
    eret

sync_el1h:
    save_state
    bl handle_sync_el1h
    load_state
    eret
    
irq_el1h:
    save_state
    bl handle_irq_el1h
    load_state
    eret

.global err_hang
err_hang: b err_hang