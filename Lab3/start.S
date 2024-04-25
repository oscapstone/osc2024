.section ".text.boot"

.global _start
.global core_timer_enable

_start:  
    // read cpu id, stop slave cores
    mov x28, x0
    bl      set_exception_vector_table
    bl      from_el2_to_el1
    //bl      core_timer_enable
    

    mrs     x1, mpidr_el1 //get cpu num
    and     x1, x1, #3 //see if cpu num is 0
    cbz     x1, 2f //cpu 0 jump to 2f (others wfe)
    // cpu id > 0, stop

1:  wfe
    b       1b

2:  // cpu id == 0

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start //ldr: load Register
    mov     sp, x1 //stack pointer before _start

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

3:  cbz     w2, 4f //see if bss is cleared
    str     xzr, [x1], #8 //xzr means zero, clear to zzero from start
    sub     w2, w2, #1 //1 count means 8bit
    cbnz    w2, 3b //if not zero go back

    // jump to C code, should not return
4:  mov x0, x28
    bl      main
    // for failsafe, halt this core too
    b       1b


from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x345 // 3c5 EL1h (SPSel = 1) with interrupt disabled, 345 enable interrupt
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1


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

    mrs x0, spsr_el1
    str x0, [sp, 16 * 16]
    mrs x0, elr_el1
    str x0, [sp, 16 * 17]
    ldp x0, x1, [sp ,16 * 0]
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
    
    ldr x0, [sp, 16 * 16]
    msr spsr_el1,x0
    ldr x0, [sp, 16 * 17]
    msr elr_el1, x0
    ldp x0, x1, [sp ,16 * 0]  // restore x0 instead of elr_el1

    add sp, sp, 32 * 9
.endm

exception_handler:
    save_all
    bl exception_entry
    load_all
    eret


.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
  b exception_handler // branch to a handler function.
  .align 7 // entry size is 0x80, .align will pad 0
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b exception_handler
  .align 7
  b interrupt_handler //uart, timer
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b exception_handler //el0 exception
  .align 7
  b interrupt_handler //el0 interrupt
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

set_exception_vector_table:
  adr x0, exception_vector_table
  msr vbar_el1, x0
  ret

#define CORE0_TIMER_IRQ_CTRL 0x40000040

core_timer_enable:
    mov x0, 1
    msr cntp_ctl_el0, x0 // enable
    mrs x0, cntfrq_el0
    msr cntp_tval_el0, x0 // set expired time
    mov x0, 2
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str w0, [x1] // unmask timer interrupt
    ret

interrupt_handler:
    save_all
    bl interrupt_handler_entry
    load_all
    eret