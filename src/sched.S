.global switch_to
switch_to:
    stp x19, x20, [x0, 16 * 0]  // save the callee-saved registers(x19-x28) and the stack pointer of the 
    stp x21, x22, [x0, 16 * 1]  // current thread into the memory pointed to by x0.
    stp x23, x24, [x0, 16 * 2]  
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]
                                // Accordingly to ARM calling conventions, regs x0 - x18 can be overwritten by the called function, 
                                // so the caller must not assume that the values of those registers will survive after a function call.
    ldp x19, x20, [x1, 16 * 0]  // load the saved values of these registers and the stack pointer from the memory pointed to by x1
    ldp x21, x22, [x1, 16 * 1]
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]
    ldr x9, [x1, 16 * 6]
    mov sp,  x9
    msr tpidr_el1, x1           // TPIDR_EL1: thread pointer/ID register for EL1, which provides a location where software executing at
    ret                         // EL1 can store thread identifying information for OS management purposes.

.global get_current_thread
get_current_thread:
    mrs x0, tpidr_el1           
    ret