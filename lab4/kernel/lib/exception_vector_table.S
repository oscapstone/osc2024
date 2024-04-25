.macro    ventry    label
    .align    7
    b    \label
.endm
// entry size is 0x80 aka align 7

// vector table should be aligned to 0x800
.align 11 
.global exception_vector_table
exception_vector_table:

    //Exception from the current EL while using SP_EL0
    ventry	sync_el1t			// Synchronous EL1t
    ventry	irq_el1t			// IRQ EL1t
    ventry	fiq_el1t			// FIQ EL1t
    ventry	error_el1t			// Error EL1t
    
    //Exception from the current EL while using SP_ELx
    ventry	sync_el1h			// Synchronous EL1h	
    ventry	irq_el1h		    // IRQ EL1h	
    ventry	fiq_el1h			// FIQ EL1h
    ventry	error_el1h			// Error EL1h

    //Exception from a lower EL and at least one lower EL is AArch64
    ventry	sync_el0_64			// Synchronous 64-bit EL0
    ventry	irq_el0_64			// IRQ 64-bit EL0
    ventry	fiq_el0_64			// FIQ 64-bit EL0
    ventry	error_el0_64		// Error 64-bit EL0
        
    //Exception from a lower EL and at least all lower EL are AArch32
    ventry	sync_el0_32			// Synchronous 32-bit EL0
    ventry	irq_el0_32			// IRQ 32-bit EL0
    ventry	fiq_el0_32			// FIQ 32-bit EL0
    ventry	error_el0_32		// Error 32-bit EL0

.macro save_all         // .macro 是像是 define 的東西
    sub sp, sp, 32 * 12         // 把sp往下移，清出一段空間因為要放register的值
    stp x0, x1, [sp ,16 * 0]    // 一次把兩個register的值放進stack
    stp x2, x3, [sp ,16 * 1]    //一個register 8 byte，所以指定offset以16為單位
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
    stp x28, x29, [sp ,16 * 14] // stp存一對，str存一個
    str x30, [sp, 16 * 15]      // x30是lr暫存器
    
    // nested interrupt
    // save current system register
    // save spsr_el1 and elr_el1
    mrs x0,  spsr_el1           //
    str x0,  [sp, 16 * 16]      // might be 16 bit align
    mrs x0,  elr_el1
    str x0,  [sp, 16 * 17]
    ldp x0, x1, [sp ,16 * 0]    // restore x0 argument register
.endm


.macro load_all
    ldp x0, x1, [sp ,16 * 0]
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
    
    // nested interrupt
    // load system register
    //
    ldr x0, [sp, 16 * 16]
    msr spsr_el1,x0
    ldr x0,  [sp, 16 * 17]
    msr elr_el1, x0
    ldp x0, x1, [sp ,16 * 0]    // restore x0 return value register
    add sp, sp, 32 * 12         // 最後把stack pointer加回去
.endm

// Exception handlers
sync_el1t:
    save_all
    mov x0,0 // for debug
    bl invalid_exception_handler
    load_all
    eret
irq_el1t:
    save_all
    mov x0,1 // for debug
    bl invalid_exception_handler
    load_all
    eret
fiq_el1t:
    save_all
    mov x0,2 // for debug
    bl invalid_exception_handler
    load_all
    eret
error_el1t:
    save_all
    mov x0,3 // for debug
    bl invalid_exception_handler
    load_all
    eret
        


sync_el1h:
    save_all
    mov x0,4 // for debug
    bl invalid_exception_handler
    load_all
    eret
irq_el1h:
    save_all
    mov x0,5 // for debug
    bl irq_handler
    load_all
    eret
fiq_el1h:
    save_all
    mov x0,6 // for debug
    bl invalid_exception_handler
    load_all
    eret
error_el1h:
    save_all
    mov x0,7 // for debug
    bl invalid_exception_handler
    load_all
    eret



sync_el0_64:
    save_all
    mov x0,8 // for debug
    bl sync_el0_64_handler
    load_all
    eret
irq_el0_64:
    save_all
    mov x0,9 // for debug
    bl irq_handler
    load_all
    eret
fiq_el0_64:
    save_all
    mov x0,10 // for debug
    bl invalid_exception_handler
    load_all
    eret
error_el0_64:
    save_all
    mov x0,11 // for debug
    bl invalid_exception_handler
    load_all
    eret



sync_el0_32:
    save_all
    mov x0,12 // for debug
    bl invalid_exception_handler
    load_all
    eret
irq_el0_32:
    save_all
    mov x0,13 // for debug
    bl invalid_exception_handler
    load_all
    eret
fiq_el0_32:
    save_all
    mov x0,14 // for debug
    bl invalid_exception_handler
    load_all
    eret
error_el0_32:
    save_all
    mov x0,15 // for debug
    bl invalid_exception_handler
    load_all
    eret

.global set_exception_vector_table
set_exception_vector_table:
    adr x1, exception_vector_table
    msr vbar_el1, x1
    ret



