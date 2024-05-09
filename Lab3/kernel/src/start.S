.section ".text.boot"

.global _start

_start:
    ldr     x1, =_dtb_ptr
    str     x0, [x1]
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3  //Check processor id
    cbz     x1, exception      //if CPU id = 0, jump to 2f
    b       hang

hang:  
    wfe
    b       hang

exception:
    bl      from_el2_to_el1
    bl      set_exception_vector_table

setup:
    ldr     x1, =_start
    mov     sp, x1
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

cleanBSS:   
    cbz     w2, master
    str     xzr, [x1], #8  // x1* = xzr, x1 += 8 (write xzr represents discard, read xzr == 0)
    sub     w2, w2, #1
    cbnz    w2, cleanBSS

master:
    bl      core_timer_enable
    bl      main
    b       hang

from_el2_to_el1:
    mov     x3, (1 << 31)   // RW, bit [31]
    msr     hcr_el2, x3     // Execution state control for lower Exception levels (EL1)
    mov     x3, 0x3c5       // EL1h (SPSel = 1) with interrupt disabled                        
    msr     spsr_el2, x3    // 0x3c5 == 0b1111000101         
    msr     elr_el2, x30    // x30 is LR
    eret                    // return to EL1

// save general registers to stack
.macro save_all
    sub sp, sp, 32 * 8
    stp x0, x1, [sp ,16 * 0]
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
.endm

// load general registers from stack
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
    add sp, sp, 32 * 8
.endm

exception_handler:
    save_all
    bl exception_entry
    load_all
    eret

irq_exception_handler:
    save_all
    bl  irq_exception_entry
    load_all
    eret
.global exception_reload_all
exception_reload_all:
    load_all
    ret

test_exception_handler:
    save_all
    bl test_entry
    load_all
    eret

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    //同級exception level遷移，使用SP_EL0。
    b exception_handler // branch to a handler function.
    .align 7 // entry size is 0x80, .align will pad 0
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    //同級exception level遷移，使用SP_ELx。
    b exception_handler
    .align 7
    b irq_exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    //ELx遷移到ELy，其中y>x且ELx處於AArch64狀態
    b exception_handler
    .align 7
    b test_exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    //ELx遷移到ELy，其中y>x且ELx處於AArch32狀態
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

set_exception_vector_table:
    adr     x1, exception_vector_table
    msr     vbar_el1, x1
    ret

#define CORE0_TIMER_IRQ_CTRL 0x40000040

.global core_timer_enable
core_timer_enable:
    mov x0, 1
    msr cntp_ctl_el0, x0                  // unmask timer interrupt
    ret

.global core_timer_disable
core_timer_disable:
    mov x0, 0
    msr cntp_ctl_el0,x0
    ret

.global core_timer_irq_enable
core_timer_irq_enable:
    mov x0, 2                       // control timer in non-secure mode
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str w0, [x1]                    // unmask timer interrupt
    ret

.global core_timer_irq_disable
core_timer_irq_disable:
    mov x0, 0                       // control timer in non-secure mode
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str w0, [x1]                    // unmask timer interrupt
    ret

.global _dtb_ptr	    //define a global variable _dtb_ptr
.section .data		    //_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	    //it defines _dtb_ptr to be a 8-byte constant with a value of 0x0