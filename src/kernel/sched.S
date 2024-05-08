.macro SAVE_REG r
    stp     x19, x20, [\r, 16 * 0]
    stp     x21, x22, [\r, 16 * 1]
    stp     x23, x24, [\r, 16 * 2]
    stp     x25, x26, [\r, 16 * 3]
    stp     x27, x28, [\r, 16 * 4]
    stp     fp, lr, [\r, 16 * 5]
    mov     x9, sp
    str     x9, [\r, 16 * 6]
.endm

.macro LOAD_REG r
    ldp     x19, x20, [\r, 16 * 0]
    ldp     x21, x22, [\r, 16 * 1]
    ldp     x23, x24, [\r, 16 * 2]
    ldp     x25, x26, [\r, 16 * 3]
    ldp     x27, x28, [\r, 16 * 4]
    ldp     fp, lr, [\r, 16 * 5]
    ldr     x9, [\r, 16 * 6]
    mov     sp,  x9
.endm

.global switch_to_regs
switch_to_regs:
    SAVE_REG    x0
    LOAD_REG    x1
    msr         TPIDR_EL1, x1
    ret

.global save_regs
save_regs:
    SAVE_REG    x0
    ret

.global load_regs
load_regs:
    LOAD_REG    x0
    ret
