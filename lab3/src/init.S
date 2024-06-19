#define CORE0_TIMER_IRQ_CTRL 0x40000040

.section ".text.boot"

// do not use x0 for saving dtb address
.globl _start
_start:                         // entry point -> to work with only the first one cpu and put all of the other cores in an endless loop.
    mrs     x2, mpidr_el1       // get process id from mpidr_el1 register to x2
    and     x2, x2, #0xFF       // Check processor id
    cbz     x2, primary         // if x2 == 0, jump to primary, if not equal to 0, means non-primary processor
    b       non_primary

non_primary:                    // if non-primary processor, to endless loop. -> easier, avoid racing...
    b       non_primary

primary: 
    bl     from_el2_to_el1

set_exception_vector_table:     // set exception vector table
    adr     x1, exception_vector_table
    msr     vbar_el1, x1        // store the address of exception_vector_table to VBAR_EL1(Vector Base Address Register)

find_bss_len:                   // find length of bss section and reset the section to zero
    adr     x2, bss_begin       // generates a PC-relative address of start point of bss section
    adr     x1, bss_end         // generates a PC-relative address of end point of bss section
    sub     x1, x1, x2          // save x1-x2 to x1 -> len of bss section

memzero:                        // initialize memory section to zero, need start point and length
	str     xzr, [x2], #8       // save 0(xzr is zero register) to x2 address and let x2 = x2+8
	subs    x1, x1, #8          // let x1 = x1-8, represent there are 8 bytes addressed
	b.gt    memzero             // if x1 > 0, jump to memzero
    b       final

final:
    ldr     x1, =_start         // load address of _start
    mov     sp, x1              // inital stack pointer to _start, because stack is grow down
    bl      kernel_main         // then jump to kernel_main function
    b       non_primary

from_el2_to_el1:                // Switch from EL2 to EL1 .
    mov     x1, (1 << 31)       // EL1 uses aarch64 (https://developer.arm.com/documentation/ddi0595/2020-12/AArch64-Registers/HCR-EL2--Hypervisor-Configuration-Register?lang=en#fieldset_0-31_31-1)
    msr     hcr_el2, x1         // Execution state control for EL2. Set HCR_EL2(Hypervisor Configuration Register)[31] -> RW to 1. Execution state for EL1 is AArch64. Execution state for EL0 is determined by the current value of PSTATE.nRW when executing at EL0.
    mov     x1, 0x345           // EL1h (SPSel = 1) with interrupt disabled (https://developer.arm.com/documentation/ddi0595/2021-03/AArch64-Registers/SPSR-EL2--Saved-Program-Status-Register--EL2-)
    // 0000 0011 1100 0101 -> 0x3c5
    //        98 7654 3210
    // set to the value of PSTATE.DAIF on taking an exception to EL2, and copied to PSTATE.DAIF on executing an exception return operation in EL2.
    // [9]: D = 1 -> disable debug exception
    // [8]: A = 1 -> disable SError interrupt 
    // [7]: I = 1 -> disable IRQ
    // [6]: F = 1 -> disable FIQ
    // [4]: M = 0 -> Execution state. Set to 0b0, the value of PSTATE.nRW, on taking an exception to EL2 from AArch64 state, and copied to PSTATE.nRW on executing an exception return operation in EL2.
    // [3:0]: M = 0b0101 -> (Execution state and selected Exception level)AArch64 Exception level and selected Stack Pointer -> 0x0101 = EL1h, meaning when return from EL2, using EL1 handler mode
    // The t suffix indicates that the SP_EL0 stack pointer is selected.
    // The h suffix indicates that the SP_ELn stack pointer is selected.
    msr     spsr_el2, x1        // set SPSR_EL2(Saved Program Status Register) register to x1
    msr     elr_el2, lr         // set ELR_EL2(Exception Link Register) = link register. This register will hold the return address to which the processor should jump when the eret is executed.
    eret                        // return to EL1(Exception Return)

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

    // regiester for nested exception
    mrs x0, spsr_el1
    str x0, [sp, 16 * 16]
    mrs x0, elr_el1
    str x0, [sp, 16 * 17]
    ldp x0, x1, [sp ,16 * 0]  // restore x0 instead of elr_el1
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
    // restore nested exception
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

irq_exception_handler:
    save_all
    bl irq_exception_handler_c
    load_all
    eret

.align 11                       // vector table should be aligned to 0x800(2 ^ 11) -> 16 entries * 0x80 = 0x800
.global exception_vector_table
exception_vector_table:
  b exception_handler           // branch to a handler function.
  .align 7                      // entry size is 0x80(2 ^ 7), .align will pad 0
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b exception_handler
  .align 7
  b irq_exception_handler
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

  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7