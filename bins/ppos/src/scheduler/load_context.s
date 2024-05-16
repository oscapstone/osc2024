//------------------------------------------------------------------------------
// fn load_context(context)
// On entry:
// *   x0 = the context struct address
//------------------------------------------------------------------------------
load_context:
    mov x8, x0
    ldp x19, x20, [x8, 16 * 0]
    ldp x21, x22, [x8, 16 * 1]
    ldp x23, x24, [x8, 16 * 2]
    ldp x25, x26, [x8, 16 * 3]
    ldp x27, x28, [x8, 16 * 4]
    ldp fp, x9, [x8, 16 * 5]
    ldr lr, [x8, 16 * 6]
    mov sp, x9
    ret

.global load_context
.type load_context function
