#include "entry.h"
#include "sys.h"
#include "peripherals/irq.h"

.section ".text"

.global set_exception_vector_table


// save general registers to stack
.macro save_all el
    sub sp, sp, 16*17
    stp x0, x1, [sp ,16 * 0]    // storing the contents of x0 and x1 to the address pointed by the stack pointer sp with an offset of 16 bytes
    stp x2, x3, [sp ,16 * 1]    // stp: Store Pair and it's used to store a pair of 64-bit (8-byte) registers to memory
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
    
    .if \el == 0                // using two distinct stack pointers for EL0 and EL1
        mrs x21, sp_el0
    .else
        add x21, sp, 16*17
    .endif

    mrs x22, elr_el1
    mrs x23, spsr_el1

    stp x30, x21, [sp ,16 * 15]
    stp x22, x23, [sp ,16 * 16]
.endm

// load general registers from stack
.macro load_all el
    ldp x22, x23, [sp ,16 * 16]
    ldp x30, x21, [sp ,16 * 15]

    .if \el == 0
        msr sp_el0, x21
    .endif

    msr elr_el1, x22
    msr spsr_el1, x23

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
    add sp, sp, 16*17
.endm

.macro ventry label
    b \label        // branch to a handler function.
    .align 7        // entry size is 0x80, .align will pad 0
.endm


.global exception_vector_table
.align 11 // vector table should be aligned to 0x800 (2^11 = 0x1000_0000_0000 = 0x800)
exception_vector_table:

    //==========================================================
    // Exception from the current EL while using SP_EL0 (exception level not changed)

    ventry SYNC_EL1T        /* Synchronous EL1t */
    ventry IRQ_EL1T         /* IRQ or vIRQ EL1t */
    ventry FIQ_EL1T         /* FIQ or vFIQ EL1t */
    ventry SERROR_EL1T      /* SError or vSError EL1t */
    

    //==========================================================
    // Exception from the current EL while using SP_ELx (exception level not changed)

    ventry SYNC_EL1H        /* Synchronous EL1h */
    ventry EL1_IRQ          /* IRQ or vIRQ EL1h */
    ventry FIQ_EL1H         /* FIQ or vFIQ EL1h */
    ventry SERROR_EL1H      /* SError or vSError EL1h */

    //==========================================================
    // Exception from a lower EL and all lower ELs are AArch64 (Moves to higher exception level)

    ventry EL0_SYNC          /* Synchronous EL0t */   
    ventry EL0_IRQ           /* IRQ or vIRQ EL0t */
    ventry FIQ_64_EL0T       /* FIQ or vFIQ EL0t */
    ventry SERROR_64_EL0T    /* SError or vSError EL0t */
    //==========================================================
    // Exception from a lower EL and all lower ELs are AArch32 Moves to higher exception level

    ventry SYNC_32_EL0T     /* Synchronous 32-bit EL0t */
    ventry IRQ_32_EL0T      /* IRQ or vIRQ 32-bit EL0t */
    ventry FIQ_32_EL0T      /* FIQ or vFIQ 32-bit EL0t */
    ventry SERROR_32_EL0T   /* SError or vSError 32-bit EL0t */


.macro handle_invalid_entry el, type
    save_all \el
    mov x0, #\type
    mrs x1, spsr_el1
    mrs x2, elr_el1
    mrs x3, esr_el1
    bl invalid_exception_entry
    b error_hang
.endm

SYNC_EL1T:
    handle_invalid_entry 1, SYNC_INVALID_EL1t

IRQ_EL1T:
    handle_invalid_entry 1, IRQ_INVALID_EL1t

FIQ_EL1T:
    handle_invalid_entry 1, FIQ_INVALID_EL1t

SERROR_EL1T:
    handle_invalid_entry 1, ERROR_INVALID_EL1t

SYNC_EL1H:
    handle_invalid_entry 1, SYNC_INVALID_EL1h

EL1_IRQ:
    save_all 1
    bl irq_handler
    load_all 1
    eret

FIQ_EL1H:
    handle_invalid_entry 1, FIQ_INVALID_EL1h

SERROR_EL1H:
    handle_invalid_entry 1, ERROR_INVALID_EL1h

EL0_SYNC: //[TODO]
    save_all 0
    mrs x25, esr_el1
    lsr x24, x25, #ESR_ELx_EC_SHIFT
    cmp x24, #ESR_ELx_EC_SVC64      // check if the exception is a system call
    b.eq el0_svc
    handle_invalid_entry 0, SYNC_ERROR  // if not a system call, handle the exception

EL0_IRQ:
    save_all 0
    bl irq_handler
    load_all 0
    eret

FIQ_64_EL0T:
    handle_invalid_entry 0, FIQ_INVALID_EL0_64

SERROR_64_EL0T:
    handle_invalid_entry 0, ERROR_INVALID_EL0_64

SYNC_32_EL0T:
    handle_invalid_entry 0, SYNC_INVALID_EL0_32

IRQ_32_EL0T:
    handle_invalid_entry 0, IRQ_INVALID_EL0_32

FIQ_32_EL0T:
    handle_invalid_entry 0, FIQ_INVALID_EL0_32

SERROR_32_EL0T:
    handle_invalid_entry 0, ERROR_INVALID_EL0_32


// creates alias
sc_nr   .req    x25                  // number of system calls
scno    .req    x26                  // syscall number
stbl    .req    x27                  // syscall table pointer

.global from_el1_to_el0
from_el1_to_el0:
    mov x2, 0x3c0
    msr spsr_el1, x2
    msr sp_el0, x1      // pass the stack pointer address from arg
    msr elr_el1, x0     // pass the program's start address from arg
    eret

.global ret_from_fork   // entry point for executing the process
ret_from_fork:
	bl	schedule_tail
	mov	x0, x20
	blr	x19 		//should never return

el0_svc:
    adr    stbl, sys_call_table      // load syscall table pointer 
    uxtw   scno, w8                  // syscall number in w8, uxtw: Unsigned Extend Word
    mov    sc_nr, #__NR_syscalls
    bl     enable_irq
    cmp    scno, sc_nr               // check upper syscall limit
    b.hs   ni_sys                   // hs: higher or same

    ldr    x16, [stbl, scno, lsl #3] // address in the syscall table, x16 = stbl[scno]
    blr    x16                       // call sys_* routine
    b      ret_from_syscall

ret_from_syscall:
    bl    disable_irq
    str   x0, [sp, #S_X0]             // returned x0 (overwritten the original x0 value on the stack)
    load_all 0
    eret

ni_sys:
    handle_invalid_entry 0, SYSCALL_ERROR

error_hang:
    b error_hang