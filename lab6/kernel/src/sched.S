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
    ldp x9, x0, [x1, 16 * 6]
    mov sp,  x9
    msr tpidr_el1, x1

    dsb ish           // ensure write has completed, lock for context switch
    msr ttbr0_el1, x0 // switch translation based address.
    tlbi vmalle1is    // invalidate all TLB entries in stage 1, el1 and inner shareable
    dsb ish           // ensure completion of TLB invalidatation
    isb               // clear pipeline

    ret

.global get_current_thread_context
get_current_thread_context:
    mrs x0, tpidr_el1          // get the current thread context address
    ret

.global set_current_thread_context
set_current_thread_context:
    msr tpidr_el1, x0          // set the current thread context address
    ret

.global flush_tlb
flush_tlb:
    dsb ish           // ensure write has completed, lock for context switch
    tlbi vmalle1is    // invalidate all TLB entries in stage 1, el1 and inner shareable
    dsb ish           // ensure completion of TLB invalidatation
    isb               // clear pipeline
    ret

.global store_context
store_context:
    stp x19, x20, [x0, 16 * 0]
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
    ldp x19, x20, [x0, 16 * 0]
    ldp x21, x22, [x0, 16 * 1]
    ldp x23, x24, [x0, 16 * 2]
    ldp x25, x26, [x0, 16 * 3]
    ldp x27, x28, [x0, 16 * 4]
    ldp fp, lr, [x0, 16 * 5]
    ldr x9, [x0, 16 * 6]
    mov sp,  x9
    ret