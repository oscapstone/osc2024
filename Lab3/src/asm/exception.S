#include "sys_regs.h"

// save general registers to stack
//https://software-dl.ti.com/codegen/docs/tiarmclang/rel1_3_0_LTS/gnu_syntax_arm_asm_language/gnu_arm_macros.html
//It defines a name that you can use as a convenient alternative to repeating the block of code.
.macro save_regs
    sub sp, sp, 32 * 8  //32 register, 1 register 8 byte
    stp x0, x1, [sp ,16 * 0]    //store x0, x1 at sp+16*0
    stp x2, x3, [sp ,16 * 1]
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
    str x30, [sp, 16 * 15]
    //https://developer.arm.com/documentation/ddi0602/2024-03/Base-Instructions/STP--Store-Pair-of-Registers-
.endm

// load general registers from stack
.macro load_regs
    //https://developer.arm.com/documentation/dui0801/g/A64-Data-Transfer-Instructions/LDP
    ldp x0, x1, [sp ,16 * 0]    //load x0, x1 from sp+16*0
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
    add sp, sp, 32 * 8
.endm

.align  11  // vector table should be aligned to 0x800 2的11次方=2048(10) = 800(16)
.global exception_vector_table
exception_vector_table:
    
    // Exception from the current EL while using SP_EL0
    // https://developer.arm.com/documentation/100933/0100/AArch64-exception-vector-table
    // entry size is 0x80, .align will pad 0 2的7次方=128(10) = 80(16) => 32 Instructions = 128 bytes
    b invalid_handler       // Synchronous
    .align 7
    b invalid_handler       // IRQ
    .align 7
    b invalid_handler       // FIQ
    .align 7
    b invalid_handler       // SError
    
    // Exception from the current EL while using SP_ELx
    .align 7
    b svc_handler           // Synchronous
    .align 7
    b irq_hander            // IRQ
    .align 7
    b invalid_handler       // FIQ
    .align 7
    b invalid_handler       // SError
    
    //Exception from a Lower EL and at least one lower EL is AArch64
    .align 7
    b svc_handler           // Synchronous
    .align 7
    b invalid_handler       // IRQ
    .align 7 
    b invalid_handler       // FIQ
    .align 7 
    b invalid_handler       // SError

    //Exception from a Lower EL and all lower ELs are AArch32
    .align 7
    b invalid_handler       // Synchronous
    .align 7
    b invalid_handler       // IRQ
    .align 7
    b invalid_handler       // FIQ
    .align 7
    b invalid_handler       // SError
    .align 7

svc_handler:
    save_regs

    mrs x0, spsr_el1    //x0 = spsr_el1 Holds the saved processor state when an exception is taken to EL1
    mrs x1, elr_el1     //x1 = elr_el1 return address
    mrs x2, esr_el1     //reson of exception
    bl  svc_router

    load_regs
    eret

invalid_hang:
	b invalid_hang

invalid_handler:
    save_regs
    bl  print_invalid_entry_message
    b   invalid_hang

.global return_to_el1
return_to_el1:
    svc 4

irq_hander:
    save_regs
    bl  irq_router
    load_regs
    eret
