.global switch_to
switch_to: // (prev, next) = (x0, x1) https://developer.arm.com/documentation/102374/0101/Procedure-Call-Standard
    stp x19, x20, [x0, 16 * 0] // store callee saved register
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]   // store sp, fp, lr  fp: the start of stack
    mov x9, sp
    str x9, [x0, 16 * 6]

    ldp x19, x20, [x1, 16 * 0] // load callee saved register
    ldp x21, x22, [x1, 16 * 1]
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]   // load fp, lr
    ldp x9, x0, [x1, 16 * 6]   // load sp, ttbr0_el1
    mov sp,  x9
    msr tpidr_el1, x1

    //armv8_a_address_translation P.28
    // https://developer.arm.com/documentation/dui0489/c/arm-and-thumb-instructions/miscellaneous-instructions/dmb--dsb--and-isb
    // dsb: Data Synchronization Barrier acts as a special kind of memory barrier. No instruction in program order after this instruction executes until this instruction completes.
    // flushes the pipeline in the processor
    // ish: DMB operation only to the inner shareable domain.
    dsb ish            // ensure write has completed    
    msr ttbr0_el1, x0  // switch translation based address.
    tlbi vmalle1is     // invalidate all TLB entries    vmalle1is: TLB invalidate by VMID, EL1, Inner Shareable.
    dsb ish            // ensure completion of TLB invalidatation
    //  https://developer.arm.com/documentation/dui0489/c/arm-and-thumb-instructions/miscellaneous-instructions/dmb--dsb--and-isb
    // isb: finish isb then fetch the instruction after isb
    isb                // clear pipeline
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

.global get_current
get_current:
    mrs x0, tpidr_el1
    ret
