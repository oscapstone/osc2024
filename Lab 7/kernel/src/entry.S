#include "syscall.h"

#define S_FRAME_SIZE        272             // size of all saved registers (16 * 17)  // 8 bytes = 64 bits
#define S_X0                0               // offset of x0 register in saved stack frame


// save general registers to stack
.macro save_all, el
    // move stack top
    sub     sp,  sp,    #S_FRAME_SIZE

    // store pair of registers
    stp     x0,  x1,    [sp, 16 * 0]
    stp     x2,  x3,    [sp, 16 * 1]
    stp     x4,  x5,    [sp, 16 * 2]
    stp     x6,  x7,    [sp, 16 * 3]
    stp     x8,  x9,    [sp, 16 * 4]
    stp     x10, x11,   [sp, 16 * 5]
    stp     x12, x13,   [sp, 16 * 6]
    stp     x14, x15,   [sp, 16 * 7]
    stp     x16, x17,   [sp, 16 * 8]
    stp     x18, x19,   [sp, 16 * 9]
    stp     x20, x21,   [sp, 16 * 10]
    stp     x22, x23,   [sp, 16 * 11]
    stp     x24, x25,   [sp, 16 * 12]
    stp     x26, x27,   [sp, 16 * 13]
    stp     x28, x29,   [sp, 16 * 14]

    .if     \el == 0
    mrs     x21,         sp_el0
    .else
    add     x21, sp,     #S_FRAME_SIZE
    .endif

    stp     x30, x21,   [sp, 16 * 15]   // push x30, stack pointer

    // push information for nested interrupt
    mrs     x22,         elr_el1
    mrs     x23,         spsr_el1

    stp     x22, x23,   [sp, 16 * 16]   // push spsr_el1, elr_el1

.endm


// load general registers from stack
.macro load_all, el

    ldp     x30, x21,   [sp, 16 * 15]
    
    .if	\el == 0
	msr     sp_el0,     x21
	.endif

    ldp     x22, x23,   [sp, 16 * 16]

    msr     elr_el1,    x22
	msr     spsr_el1,   x23

    ldp     x0,  x1,    [sp, 16 * 0]
    ldp     x2,  x3,    [sp, 16 * 1]
    ldp     x4,  x5,    [sp, 16 * 2]
    ldp     x6,  x7,    [sp, 16 * 3]
    ldp     x8,  x9,    [sp, 16 * 4]
    ldp     x10, x11,   [sp, 16 * 5]
    ldp     x12, x13,   [sp, 16 * 6]
    ldp     x14, x15,   [sp, 16 * 7]
    ldp     x16, x17,   [sp, 16 * 8]
    ldp     x18, x19,   [sp, 16 * 9]
    ldp     x20, x21,   [sp, 16 * 10]
    ldp     x22, x23,   [sp, 16 * 11]
    ldp     x24, x25,   [sp, 16 * 12]
    ldp     x26, x27,   [sp, 16 * 13]
    ldp     x28, x29,   [sp, 16 * 14]

    // restore stack top
    add     sp,  sp,    #S_FRAME_SIZE
.endm


.globl err_hang
err_hang: 
    b       err_hang

.macro handle_invalid_entry el, type
	save_all    \el
	// mov	    x0,     #\type
	// mrs	    x1,     esr_el1
	// mrs	    x2,     elr_el1
	// bl	    show_invalid_entry_message
	b	    err_hang
.endm




.macro    exception_entry    label
    .align  7           // entry size is 0x80, should be aligned to 2^7
    b    \label
.endm


                        // vbar_el1[10:0] bits are reserved 
    .align 11           // vector table should be aligned to 2^11 (0x800)
    .global exception_vector_table
exception_vector_table:
    // Exception from the current EL while using SP_EL0
    exception_entry     exception_invalid   // Synchronous EL1t (invalid)
    exception_entry     exception_invalid   // IRQ EL1t (invalid)
    exception_entry     exception_invalid   // FIQ EL1t (invalid)
    exception_entry     exception_invalid   // SError EL1t (invalid)

    // Exception from the current EL while using SP_ELx
    exception_entry     el1_sync            // Synchronous EL1h
    exception_entry     el1_irq             // IRQ EL1h
    exception_entry     exception_invalid   // FIQ EL1h (invalid)
    exception_entry     exception_invalid   // SError EL1h (invalid)

    // Exception from a lower EL and at least one lower EL is AArch64
    exception_entry     el0_sync            // Synchronous 64-bit EL0
    exception_entry     el0_irq             // IRQ 64-bit EL0
    exception_entry     exception_invalid   // FIQ 64-bit EL0 (invalid)
    exception_entry     exception_invalid   // Error 64-bit EL0 (invalid)

    // Exception from a lower EL and all lower EL are AArch32
    exception_entry     exception_invalid   // Synchronous 32-bit EL0 (invalid)
    exception_entry     exception_invalid   // IRQ 64-bit EL0 (invalid)
    exception_entry     exception_invalid   // FIQ 64-bit EL0 (invalid)
    exception_entry     exception_invalid   // Error 64-bit EL0 (invalid)


exception_invalid:                          // default exception router
    save_all  0
    bl      exception_invalid_handler       // c code
    load_all  0
    eret                                    // EL1 -> EL0


el1_sync:
    save_all  1
    bl      exception_el1_sync_handler
    load_all  1
    eret

    
el1_irq:
    save_all  1
    bl      exception_el1_irq_handler
    load_all  1
    eret


#define ESR_ELx_EC_SHIFT        26
#define ESR_ELx_EC_SVC64        0x15
#define NR_SYSCALLS             11

el0_sync:
    save_all  0
    // bl      exception_el0_sync_handler

    mrs	    x25,        esr_el1	                // read the syndrome register
    lsr	    x24, x25,   #ESR_ELx_EC_SHIFT       // exception class
    cmp	    x24,        #ESR_ELx_EC_SVC64       // SVC in 64-bit state
    b.eq    el0_svc
    handle_invalid_entry 0, SYNC_ERROR

sc_nr   .req    x25	                            // number of system calls
scno    .req    x26                             // syscall number
stbl    .req    x27                             // syscall table pointer

el0_svc:
    adr	    stbl,       syscall_table           // load syscall table pointer
    uxtw    scno,       w8                      // syscall number in w8
    mov	    sc_nr,      #NR_SYSCALLS
    bl      enable_irq
    cmp     scno,       sc_nr                   // check upper syscall limit
    b.hs	ni_sys

    ldr     x16,        [stbl, scno, lsl #3]    // address in the syscall table
    blr     x16                                 // call sys_* routine
    b       ret_from_syscall
ni_sys:
    handle_invalid_entry 0, SYSCALL_ERROR
ret_from_syscall:
    bl	    disable_irq
    str	    x0,         [sp, #S_X0]             // returned x0
    load_all  0
    eret


el0_irq:
    save_all  0
    bl      exception_el0_irq_handler
    load_all  0
    eret


    .global set_exception_vector_table
set_exception_vector_table:
    adr     x0, exception_vector_table
    msr     vbar_el1, x0
    ret
    // vbar_el1: Vector Base Address Register (EL1)
    //           Holds the exception base address for any 
    //           exception that is taken to EL1.


.globl ret_from_fork
ret_from_fork:
    bl      schedule_tail
	cbz	    x19,    ret_to_user			// not a kernel thread
    mov	    x0,     x20
	blr	    x19

ret_to_user:
	bl      disable_irq				
	load_all  0 
    eret