// switch_to(prev, next), as x0 is parameter register 1(prev), and x1 is parameter register 2(next)
// tpidr_el1 stores the per_cpu offset of current CPU, Provides a location where software executing at EL1 can store thread identifying information
// about which registers need to save: https://developer.arm.com/documentation/102374/0101/Procedure-Call-Standard
// usually we write all callee-saved registers into current tpidr_el1, then load all callee-saved registers from x1 and set tpidr_el1 to x1
.global switch_to
switch_to:
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]

    ldp x19, x20, [x1, 16 * 0]
    ldp x21, x22, [x1, 16 * 1]
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]
    ldr x9, [x1, 16 * 6]
    mov sp,  x9
    msr tpidr_el1, x1
    ret

.global store_context
store_context:
    stp x19, x20, [x0, 16 * 0] // put into x0
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]
    ret

.global load_context
load_context:
    ldp x19, x20, [x0, 16 * 0] // get from x0
    ldp x21, x22, [x0, 16 * 1]
    ldp x23, x24, [x0, 16 * 2]
    ldp x25, x26, [x0, 16 * 3]
    ldp x27, x28, [x0, 16 * 4]
    ldp fp, lr, [x0, 16 * 5]
    ldr x9, [x0, 16 * 6]
    mov sp,  x9
    ret

.global get_current
get_current:
    mrs x0, tpidr_el1
    ret

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
    ldp x30, x0, [sp, 16 * 15]
    msr spsr_el1, x0
    // load information when interrupted task return(used when nested interrupt occurs)
    ldp x0, x1, [sp, 16 * 16]
    msr elr_el1, x0
    msr sp_el0, x1
    // restore x0 and x1(as we just use it to restore the information of spsr and elr)
    ldp x0, x1, [sp ,16 * 0]
    add sp, sp, 16 * 17
.endm

.global ret_from_fork
ret_from_fork:
    cbz x19, 1f         // not kernel thread
    mov x0, x20
    blr x19
1:
    load_all
    eret