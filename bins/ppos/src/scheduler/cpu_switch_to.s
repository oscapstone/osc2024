//------------------------------------------------------------------------------
// fn cpu_switch_to(prev, next)
// On entry:
// *   x0 = previous Task struct (must be preserved across the switch)
// *   x1 = next Task struct
//------------------------------------------------------------------------------
cpu_switch_to:
    mov x8, x0
    mov x9, sp
    stp x19, x20, [x8, 16 * 0]
    stp x21, x22, [x8, 16 * 1]
    stp x23, x24, [x8, 16 * 2]
    stp x25, x26, [x8, 16 * 3]
    stp x27, x28, [x8, 16 * 4]
    stp fp, x9, [x8, 16 * 5]
    str lr, [x8, 16 * 6]

    mov x8, x1
    ldp x19, x20, [x8, 16 * 0]
    ldp x21, x22, [x8, 16 * 1]
    ldp x23, x24, [x8, 16 * 2]
    ldp x25, x26, [x8, 16 * 3]
    ldp x27, x28, [x8, 16 * 4]
    ldp fp, x9, [x8, 16 * 5]
    ldr lr, [x8, 16 * 6]
    mov sp,  x9
    
    // ensure kernel space interrupt is enable
    msr DAIFClr, 0xf
    ret

.global cpu_switch_to
.type cpu_switch_to function
