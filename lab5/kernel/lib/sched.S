
// tpidr_el1 : per cpu offset
// MSB 2 bit : thread id

// save callee-saved register

.global switch_to
switch_to:
    stp x19, x20, [x0, 16 * 0]  // 將 x19, x20 存到 x0 指向的位置
    stp x21, x22, [x0, 16 * 1]  // 將 x21, x22 存到 x0 + 16 的位置
    stp x23, x24, [x0, 16 * 2]  // 將 x23, x24 存到 x0 + 32 的位置
    stp x25, x26, [x0, 16 * 3]  // 將 x25, x26 存到 x0 + 48 的位置
    stp x27, x28, [x0, 16 * 4]  // 將 x27, x28 存到 x0 + 64 的位置
    stp fp, lr, [x0, 16 * 5]    // 將fp和lr存到 x0 + 80 的位置
    mov x9, sp                  // 將sp的值移動到 x9
    str x9, [x0, 16 * 6]        // 將 x9 的值存到 x0 + 96 的位置 (sp)

    ldp x19, x20, [x1, 16 * 0]  // 從 x1 指向的位置載入 x19, x20
    ldp x21, x22, [x1, 16 * 1]  // 從 x1 + 16 的位置載入 x21, x22
    ldp x23, x24, [x1, 16 * 2]  // 從 x1 + 32 的位置載入 x23, x24
    ldp x25, x26, [x1, 16 * 3]  // 從 x1 + 48 的位置載入 x25, x26
    ldp x27, x28, [x1, 16 * 4]  // 從 x1 + 64 的位置載入 x27, x28
    ldp fp, lr, [x1, 16 * 5]    // 從 x1 + 80 的位置載入 fp和lr
    ldr x9, [x1, 16 * 6]        // 從 x1 + 96 的位置載入sp到 x9
    mov sp,  x9                 // 將 x9 的值移動到堆疊指標 (sp)
    msr tpidr_el1, x1           // 將 x1 的值載入到 TPIDR_EL1 (設置current thread context)
    ret                         // return

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
