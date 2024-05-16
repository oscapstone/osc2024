//------------------------------------------------------------------------------
// fn store_context(context)
// On entry:
// *   x0 = the context struct address
//------------------------------------------------------------------------------
store_context:
    mov x8, x0
    mov x9, sp
    stp x19, x20, [x8, 16 * 0]
    stp x21, x22, [x8, 16 * 1]
    stp x23, x24, [x8, 16 * 2]
    stp x25, x26, [x8, 16 * 3]
    stp x27, x28, [x8, 16 * 4]
    stp fp, x9, [x8, 16 * 5]
    str lr, [x8, 16 * 6]
    ret

.global store_context
.type store_context function
