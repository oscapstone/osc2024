.section ".text"

.global set_exception_vector_table

// save general registers to stack
.macro save_all
    sub sp, sp, 32 * 9
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
    str x30, [sp, 16 * 15]
    mrs x28, spsr_el1
    mrs x29, elr_el1
    stp x28, x29, [sp, 16 * 16]
.endm

// load general registers from stack
.macro load_all
    ldp x28, x29, [sp, 16 * 16]
    msr spsr_el1, x28
    msr elr_el1, x29
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
    add sp, sp, 32 * 9
.endm

.macro ventry label
    b \label        // branch to a handler function.
    .align 7        // entry size is 0x80, .align will pad 0
.endm

.macro handler_warpper label branch_to type
\label:
    save_all
    mov x0, #\type
    mrs x1, spsr_el1
    mrs x2, elr_el1
    mrs x3, esr_el1
    bl \branch_to
    load_all
    eret
.endm

.align 11 // aligned to 0x800 (2^11 = 0x1000_0000_0000 = 0x800)
exception_vector_table:
    // Exception from the current EL while using SP_EL0 (exception level not changed)
    ventry SYNC_EL1T        /* Synchronous EL1t */
    ventry IRQ_EL1T         /* IRQ or vIRQ EL1t */
    ventry FIQ_EL1T         /* FIQ or vFIQ EL1t */
    ventry SERROR_EL1T      /* SError or vSError EL1t */
    
    // Exception from the current EL while using SP_ELx (exception level not changed)
    ventry SYNC_EL1H        /* Synchronous EL1h */
    ventry IRQ_EL1H         /* IRQ or vIRQ EL1h */
    ventry FIQ_EL1H         /* FIQ or vFIQ EL1h */
    ventry SERROR_EL1H      /* SError or vSError EL1h */

    // Exception from a lower EL and all lower ELs are AArch64 (Moves to higher exception level)
    ventry SYNC_64_EL0T     /* Synchronous EL0t */
    ventry IRQ_64_EL0T      /* IRQ or vIRQ EL0t */
    ventry FIQ_64_EL0T      /* FIQ or vFIQ EL0t */
    ventry SERROR_64_EL0T   /* SError or vSError EL0t */

    // Exception from a lower EL and all lower ELs are AArch32 Moves to higher exception level
    ventry SYNC_32_EL0T     /* Synchronous 32-bit EL0t */
    ventry IRQ_32_EL0T      /* IRQ or vIRQ 32-bit EL0t */
    ventry FIQ_32_EL0T      /* FIQ or vFIQ 32-bit EL0t */
    ventry SERROR_32_EL0T   /* SError or vSError 32-bit EL0t */

// using wrapper to define the handler
handler_warpper     SYNC_EL1T           default_exception_entry     0
handler_warpper     IRQ_EL1T            default_exception_entry     1
handler_warpper     FIQ_EL1T            default_exception_entry     2
handler_warpper     SERROR_EL1T         default_exception_entry     3
        
handler_warpper     SYNC_EL1H           default_exception_entry     4
handler_warpper     IRQ_EL1H            el1_irq_entry               5
handler_warpper     FIQ_EL1H            default_exception_entry     6
handler_warpper     SERROR_EL1H         default_exception_entry     7
        
handler_warpper     SYNC_64_EL0T        svc_exception_entry         8
handler_warpper     IRQ_64_EL0T         el0_irq_entry               9
handler_warpper     FIQ_64_EL0T         default_exception_entry     10
handler_warpper     SERROR_64_EL0T      default_exception_entry     11
        
handler_warpper     SYNC_32_EL0T        default_exception_entry     12
handler_warpper     IRQ_32_EL0T         default_exception_entry     13
handler_warpper     FIQ_32_EL0T         default_exception_entry     14
handler_warpper     SERROR_32_EL0T      default_exception_entry     15

set_exception_vector_table:
    adr x0, exception_vector_table  // get the base address of the exception vector table
    msr vbar_el1, x0                // set the base address of the exception vector table
    ret


// page 2107
.global from_el2_to_el1
from_el2_to_el1:
    mov x0, (1 << 31)   // EL1 uses aarch64
    msr hcr_el2, x0     // msr: Status Register <-- Register (page 1924)
    mov x0, 0x3c5       // EL1h (SPSel = 1) with interrupt disabled (D = 1, A = 1, I = 1, F = 1, M[3:0] = 5) EL1h// TDO: check
    msr spsr_el2, x0    // Save the current processor's state
    msr elr_el2, lr     // Save the exception return address
    eret                // return to EL1 (use eret to return from the exception)

.global from_el1_to_el0
from_el1_to_el0:
    mov x2, 0x3c0
    msr spsr_el1, x2
    msr sp_el0, x1      // pass the stack pointer address from arg
    msr elr_el1, x0     // pass the program's start address from arg
    eret

