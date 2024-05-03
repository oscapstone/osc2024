// save general registers to stack
.macro save_all
    // move stack top
    sub     sp,  sp,  36 * 8        // 8 bytes = 64 bits

    // store pair of registers
    stp     x0,  x1,  [sp ,16 * 0]
    stp     x2,  x3,  [sp ,16 * 1]
    stp     x4,  x5,  [sp ,16 * 2]
    stp     x6,  x7,  [sp ,16 * 3]
    stp     x8,  x9,  [sp ,16 * 4]
    stp     x10, x11, [sp ,16 * 5]
    stp     x12, x13, [sp ,16 * 6]
    stp     x14, x15, [sp ,16 * 7]
    stp     x16, x17, [sp ,16 * 8]
    stp     x18, x19, [sp ,16 * 9]
    stp     x20, x21, [sp ,16 * 10]
    stp     x22, x23, [sp ,16 * 11]
    stp     x24, x25, [sp ,16 * 12]
    stp     x26, x27, [sp ,16 * 13]
    stp     x28, x29, [sp ,16 * 14]
    str     x30,      [sp, 16 * 15]

    // push information for nested interrupt
    mrs     x0,       spsr_el1
    str     x0,       [sp, 16 * 16] // push spsr_el1
    mrs     x0,       elr_el1
    str     x0,       [sp, 16 * 17] // push elr_el1
    // restore x0
    ldr     x0,       [sp ,16 * 0] 
.endm


// load general registers from stack
.macro load_all
    ldp     x0,  x1,  [sp ,16 * 0]
    ldp     x2,  x3,  [sp ,16 * 1]
    ldp     x4,  x5,  [sp ,16 * 2]
    ldp     x6,  x7,  [sp ,16 * 3]
    ldp     x8,  x9,  [sp ,16 * 4]
    ldp     x10, x11, [sp ,16 * 5]
    ldp     x12, x13, [sp ,16 * 6]
    ldp     x14, x15, [sp ,16 * 7]
    ldp     x16, x17, [sp ,16 * 8]
    ldp     x18, x19, [sp ,16 * 9]
    ldp     x20, x21, [sp ,16 * 10]
    ldp     x22, x23, [sp ,16 * 11]
    ldp     x24, x25, [sp ,16 * 12]
    ldp     x26, x27, [sp ,16 * 13]
    ldp     x28, x29, [sp ,16 * 14]
    ldr     x30,      [sp, 16 * 15]

    // pop information for nested interrupt
    ldr     x0,       [sp, 16 * 16] 
    msr     spsr_el1, x0            // pop spsr_el1
    ldr     x0,       [sp, 16 * 17]
    msr     elr_el1,  x0            // pop elr_el1
    // restore x0
    ldr     x0,       [sp ,16 * 0]

    // restore stack top
    add     sp,  sp,  36 * 8
.endm

.macro    exception_entry    label
    .align  7           // entry size is 0x80, should be aligned to 2^7
    b    \label
.endm


                        // vbar_el1[10:0] bits are reserved 
    .align 11           // vector table should be aligned to 2^11 (0x800)
    .global exception_vector_table
exception_vector_table:
    // Exception from the current EL while using SP_EL0
    exception_entry     exception_invalid   // Synchronous EL1t (invalid)
    exception_entry     exception_invalid   // IRQ EL1t (invalid)
    exception_entry     exception_invalid   // FIQ EL1t (invalid)
    exception_entry     exception_invalid   // SError EL1t (invalid)

    // Exception from the current EL while using SP_ELx
    exception_entry     el1_sync            // Synchronous EL1h
    exception_entry     el1_irq             // IRQ EL1h
    exception_entry     exception_invalid   // FIQ EL1h (invalid)
    exception_entry     exception_invalid   // SError EL1h (invalid)

    // Exception from a lower EL and at least one lower EL is AArch64
    exception_entry     el0_sync            // Synchronous 64-bit EL0
    exception_entry     el0_irq             // IRQ 64-bit EL0
    exception_entry     exception_invalid   // FIQ 64-bit EL0 (invalid)
    exception_entry     exception_invalid   // Error 64-bit EL0 (invalid)

    // Exception from a lower EL and all lower EL are AArch32
    exception_entry     exception_invalid   // Synchronous 32-bit EL0 (invalid)
    exception_entry     exception_invalid   // IRQ 64-bit EL0 (invalid)
    exception_entry     exception_invalid   // FIQ 64-bit EL0 (invalid)
    exception_entry     exception_invalid   // Error 64-bit EL0 (invalid)


exception_invalid:                          // default exception router
    save_all
    bl      exception_invalid_handler       // c code
    load_all
    eret                                    // EL1 -> EL0


el1_sync:
    save_all
    bl      exception_el1_sync_handler
    load_all
    eret

    
el1_irq:
    save_all
    bl      exception_el1_irq_handler
    load_all
    eret


el0_sync:
    save_all
    bl      exception_el0_sync_handler
    load_all
    eret


el0_irq:
    save_all
    bl      exception_el0_irq_handler
    load_all
    eret


    .global set_exception_vector_table
set_exception_vector_table:
    adr     x0, exception_vector_table
    msr     vbar_el1, x0
    ret
    // vbar_el1: Vector Base Address Register (EL1)
    //           Holds the exception base address for any 
    //           exception that is taken to EL1.
