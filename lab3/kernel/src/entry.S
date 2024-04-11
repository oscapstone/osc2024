// save general registers to stack
// 32 * 9 = 288: 須保存x0~x29+x30+spsr_el1+elr_el1＝33個regs(1個reg：8bytes=64bits)，總共要264bytes的空間
// 而stack須align 32 bytes，故取到最接近264的32的倍數＝32*9＝288 bytes
// stp: [x0, x1] -> [sp ,16 * 0]
// ldp: [x0, x1] <- [sp ,16 * 0]
// mrs: [x0] <- [spsr_el1]
// msr: [spsr_el1] <- [x0]
.macro save_all
    sub sp, sp, 32 * 9             //32個暫存器，一個暫存器8 bytes(64 bits)，sp故一次跳16 bytes
    stp x0, x1, [sp ,16 * 0]       // store pair (stp) of registers：將兩個64bits的值x0和x1暫存器中的值存儲到stack中
    stp x2, x3, [sp ,16 * 1]       // 16*1 :代表的是在stack上的相對位置（offsets）
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]          // 這行是為了將LR(x30)暫存器單獨保存在stack上
    // information for nested interrupt
    mrs x0, spsr_el1          // 這行將當前異常等級EL1的SPSR的值存入x0
    str x0, [sp, 16 * 16]     // 這是為了在stack上保存SPSR的值:將x0的值移入堆疊指針(Stack Pointer, sp) // 指向的地址加上一個offset(16 * 16 字節，即256字節)，指到下個存入的起始地址
    mrs x0, elr_el1           // 將EL1的ELR（Exception Link Register）的值移入x0。ELR中存儲了當異常發生時，程序應該返回執行的地址
    str x0, [sp, 16 * 17]     // 這是為了在stack上保存ELR的值
    ldp x0, x1, [sp ,16 * 0]  // 恢復x0,x1原本的內容 // Load Pair（ldp）:這行用來從sp指向的記憶體地址開始，一次讀取兩個64bits的值，並分別載入到x0和x1暫存器中// restore x0,x1 instead of elr_el1
.endm

// load general registers from stack
// ldp: [x0, x1] <- [sp ,16 * 0]
// msr: [spsr_el1] <- x0
.macro load_all
    // 恢復暫存器的值
    ldp x0, x1, [sp ,16 * 0]        // 把stack的東西load回x0與x1
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    // information for nested interrupt
    ldr x0, [sp, 16 * 16] // 將[sp, 16 * 16]的東西還給spsr_el1
    msr spsr_el1, x0       
    ldr x0, [sp, 16 * 17] // 將[sp, 16 * 17]的東西還給elr_el1
    msr elr_el1, x0
    ldp x0, x1, [sp ,16 * 0]  // restore x0,x1 instead of elr_el1
    add sp, sp, 32 * 9
.endm

// 跳轉指令 接受一個參數：label
// label前面加上斜線是為了表示使用ventry這個macro參數
.macro    ventry    label
    .align    7   // 對齊128 bits(16 bytes)// entry should be aligned to 0x80 (2^7) (adderss)
    b    \label   // branch指令，它在異常發生時會跳轉到指定的標籤（\label）處執行相應的異常處理代碼
.endm

.align 11         // 異常向量表的起始地址會是 2048 的整數倍 // vector table should be aligned to 0x800 (2^11) (adderss)
.global exception_vector_table

// exception_vector_table definition
// https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L367
exception_vector_table:
    //Exception from the current EL while using SP_EL0
    ventry    el1t_sync_invalid        // Synchronous EL1t
    ventry    el1t_irq_invalid         // IRQ EL1t
    ventry    el1t_fiq_invalid         // FIQ EL1t
    ventry    el1t_error_invalid       // Error EL1t

    //Exception from the current EL while using SP_ELx
    ventry    el1h_sync                // Synchronous EL1h
    ventry    el1h_irq                 // IRQ EL1h
    ventry    el1h_fiq_invalid         // FIQ EL1h
    ventry    el1h_error_invalid       // Error EL1h

    //Exception from a lower EL and at least one lower EL is AArch64
    ventry    el0_sync_64             // Synchronous 64-bit EL0
    ventry    el0_irq_64              // IRQ 64-bit EL0
    ventry    el0_fiq_invalid_64      // FIQ 64-bit EL0
    ventry    el0_error_invalid_64    // Error 64-bit EL0

    //Exception from a lower EL and at least all lower EL are AArch32
    ventry    el0_sync_invalid_32     // Synchronous 32-bit EL0
    ventry    el0_irq_invalid_32      // IRQ 32-bit EL0
    ventry    el0_fiq_invalid_32      // FIQ 32-bit EL0
    ventry    el0_error_invalid_32    // Error 32-bit EL0

// 每個異常處理程序中的 mov x0, <number> 是一個數字，
// 用於標識不同類型的異常。這樣，異常處理路由函數可以通過檢查 x0 的值來判斷是哪種異常發生，並根據需要執行相應的處理。
el1t_sync_invalid:
    save_all
    mov x0,0                          //這行會將0移動到x0 中。這裡x0用於傳遞信息給invalid_exception_router函數，比如異常類型或錯誤碼。
    bl invalid_exception_router
    load_all
    eret
el1t_irq_invalid:
    save_all
    mov x0,1
    bl invalid_exception_router
    load_all
    eret
el1t_fiq_invalid:
    save_all
    mov x0,2
    bl invalid_exception_router
    load_all
    eret
el1t_error_invalid:
    save_all
    mov x0,3
    bl invalid_exception_router
    load_all
    eret


el1h_sync:
    save_all
    mov x0,4
    bl invalid_exception_router
    load_all
    eret
el1h_irq:
    save_all
    mov x0,5
    bl el1h_irq_router
    load_all
    eret
el1h_fiq_invalid:
    save_all
    mov x0,6
    bl invalid_exception_router
    load_all
    eret
el1h_error_invalid:
    save_all
    mov x0,7
    bl invalid_exception_router
    load_all
    eret

el0_sync_64:
    save_all
    mov x0,8
    bl el0_sync_router
    load_all
    eret
el0_irq_64:
    save_all
    mov x0,9
    bl el0_irq_64_router
    load_all
    eret
el0_fiq_invalid_64:
    save_all
    mov x0,10
    bl invalid_exception_router
    load_all
    eret
el0_error_invalid_64:
    save_all
    mov x0,11
    bl invalid_exception_router
    load_all
    eret


el0_sync_invalid_32:
    save_all
    mov x0,12
    bl invalid_exception_router
    load_all
    eret
el0_irq_invalid_32:
    save_all
    mov x0,13
    bl invalid_exception_router
    load_all
    eret
el0_fiq_invalid_32:
    save_all
    mov x0,14
    bl invalid_exception_router
    load_all
    eret
el0_error_invalid_32:
    save_all
    mov x0,15
    bl invalid_exception_router
    load_all
    eret
