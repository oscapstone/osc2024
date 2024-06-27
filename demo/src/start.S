.section ".text.boot"

#define CORE0_TIMER_IRQ_CTRL 0x40000040

.global _start

_start:
    // bootloader loads a dtb into memory and passes the loading address specified at register x0 to the kernel. 
    // store x0 value for dtb
    mov     x10, x0

    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbnz    x1, halt
    // cpu id > 0, stop

change_ell_stat:
    bl from_el2_to_el1
    b init_sp

//Set stack pointer before code starts
init_sp:
    ldr x1, =_start
    mov sp, x1
    b set_exception_vector_table

set_exception_vector_table:
    adr x0, exception_vector_table
    msr vbar_el1, x0
    b bss_init

// cpu id == 0
bss_init:  
    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

clear_bss_memory:  
    cbz     w2, enable_Core_Timer
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, clear_bss_memory

enable_Core_Timer:
    bl core_timer_enable
    b jump_to_main

// jump to C code, should not return
jump_to_main:  
    mov     x0, x10

    bl      main
    // for failsafe, halt this core too
    b       halt

from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3C5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1

halt:  
    wfe
    b       halt 

//exception
.macro exception_entry function
    b      \function
    .align 7            // entry size is 0x80, .align will pad 0
.endm


.align 11 // vector table should be aligned to 0x800
.global exception_vector_table

exception_vector_table:
// Exception from the current EL while using SP_EL0
    exception_entry showinfo_exception
    exception_entry showinfo_exception
    exception_entry showinfo_exception
    exception_entry showinfo_exception

// Exception from the current EL while using SP_ELx
    exception_entry showinfo_exception
    exception_entry irq_exception                 // el1
    exception_entry showinfo_exception
    exception_entry showinfo_exception

// Exception from a lower EL and at least one lower EL is AArch64
    exception_entry showinfo_exception
    exception_entry irq_exception             // el0
    exception_entry showinfo_exception
    exception_entry showinfo_exception

// Exception from a lower EL and all lower EL are AArch32
    exception_entry showinfo_exception
    exception_entry showinfo_exception
    exception_entry showinfo_exception
    exception_entry showinfo_exception
 

// save general registers to stack
.macro save_all
    sub sp, sp, 32 * 9
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

    mrs x0, spsr_el1
    mrs x1, elr_el1
    stp x0, x1, [sp, 16 * 16]
    // restore x0 and x1(as we just use it to save the information of spsr and elr)
    ldp x0, x1, [sp, 16 * 0] 

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

    ldp x0, x1, [sp, 16 * 16]
    msr spsr_el1, x0
    msr elr_el1, x1
    // restore x0 and x1(as we just use it to restore the information of spsr and elr)
    ldp x0, x1, [sp ,16 * 0]
    add sp, sp, 32 * 9
.endm


showinfo_exception:
    save_all
    bl showinfo_exception_handler
    load_all
    eret

irq_exception:
    save_all
    bl irq_exception_handler
    load_all
    eret


core_timer_enable:
    mov x0, 1
    msr cntp_ctl_el0, x0 // enable
    mrs x0, cntfrq_el0
    msr cntp_tval_el0, x0 // set expired time
    mov x0, 2
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str w0, [x1] // unmask timer interrupt
    ret


