.macro SAVE_REG r0
    stp     x19, x20, [\r0, 16 * 0]
    stp     x21, x22, [\r0, 16 * 1]
    stp     x23, x24, [\r0, 16 * 2]
    stp     x25, x26, [\r0, 16 * 3]
    stp     x27, x28, [\r0, 16 * 4]
    stp     fp, lr, [\r0, 16 * 5]
    mov     x9, sp
    str     x9, [\r0, 16 * 6]
.endm

.macro LOAD_REG r0 r1
    ldp     x19, x20, [\r0, 16 * 0]
    ldp     x21, x22, [\r0, 16 * 1]
    ldp     x23, x24, [\r0, 16 * 2]
    ldp     x25, x26, [\r0, 16 * 3]
    ldp     x27, x28, [\r0, 16 * 4]
    ldp     fp, lr, [\r0, 16 * 5]
    ldr     x9, [\r0, 16 * 6]
    mov     sp,  x9
    msr     TPIDR_EL1, \r1
.endm

.macro LOAD_PGD r1
    dsb     ISH             // ensure write has completed
    msr     TTBR0_EL1, \r1  // switch translation based address.
    tlbi    VMALLE1IS       // invalidate all TLB entries
    dsb     ISH             // ensure completion of TLB invalidatation
    isb                     // clear pipeline
.endm

.global switch_to_regs
switch_to_regs:
    SAVE_REG    x0
    LOAD_REG    x1 x2
    LOAD_PGD    x3
    ret

.global save_regs
save_regs:
    SAVE_REG    x0
    ret

.global load_regs
load_regs:
    LOAD_REG    x0 x1
    LOAD_PGD    x2
    ret
