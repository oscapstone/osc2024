#define THREAD_CPU_CONTEXT_OFFSET  0 


.globl cpu_switch_to
cpu_switch_to:

    mov     x10,        #THREAD_CPU_CONTEXT_OFFSET

    // store current thread callee-saved registers
    add     x8,  x0,    x10
    mov     x9,         sp
    stp	    x19, x20,   [x8], #0x10
    stp	    x21, x22,   [x8], #0x10
    stp	    x23, x24,   [x8], #0x10
    stp	    x25, x26,   [x8], #0x10
    stp	    x27, x28,   [x8], #0x10
    stp     fp,   x9,   [x8], #0x10
    str     lr,         [x8]


    // restore next thread callee-saved registers
    add     x8,  x1,    x10
    ldp     x19, x20,   [x8], #0x10
    ldp	    x21, x22,   [x8], #0x10
    ldp	    x23, x24,   [x8], #0x10
    ldp	    x25, x26,   [x8], #0x10
    ldp	    x27, x28,   [x8], #0x10
    ldp     fp,  x9,    [x8], #0x10
    ldr     lr,         [x8]
    mov     sp,         x9
    
    ret