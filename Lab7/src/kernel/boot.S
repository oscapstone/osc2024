// A53 document:https://developer.arm.com/documentation/ddi0500/j/System-Control/AArch32-register-descriptions/Multiprocessor-Affinity-Register?lang=en
// where this code belongs(.text section) in the compiled binary.
.section ".text.boot"
// 
#define CORE0_TIMER_IRQ_CTRL 0x40000040
// Make _start global.
.global _start

_start:
  // only the primary core runs (core 0), and the secondary cores are awaiting in a spin loop.
  // mpidr_el1: 64 bits Multiprocessor Affinity Register
  mrs x1, mpidr_el1 // Read MPIDR_EL1 into X1
  // Gat value [2:0] which is the CPU id(it's hard to understand in official spec, it's should try to sat that if 0x0,0x1 will invidually appear when it's switch to core 0 and 1 )
  // 0xFF should also OK?
  and x1, x1, #0xFF
  // If it's not core 0, send to busy waiting(Compare Branch Zero?)
  // xf(b) : branches to the first found label "x" searching "forward" for "f" or "backward" for "b".(GNU specific) see:https://stackoverflow.com/questions/27353096/1b-and-1f-in-gnu-assembly
  cbz x1, 1f

// if it's not cpu0
halt: 
  // Wait for event
  // When the C function returns, it enters the halt procedure where it loops forever doing nothing.
  wfe
  b halt

  // if it's cpu0
1:  
  // store the address of _dtb_addr
  // then load the dtb loading address(x0) into x1(_dtb_addr)
  ldr x1, =_dtb_addr
  str x0, [x1]
  // branch and link. change from EL2 to EL1 before any initialization start(this should after the device tree address is obtained as it will change x0)
  bl from_el2_to_el1

  // Set stack pointer before code starts
  ldr x1, =_start
  mov sp, x1

set_exception_vector_table:
  adr x0, exception_vector_table  // save relative address(PC) of exception_vector_table to x0
  msr vbar_el1, x0                // Vector Base Address Register: Holds the vector base address for any exception that is taken to EL1.

  // clear bss
  // load bss start location to x1(64bit register)
  ldr x1, =__bss_start
  // load bss size into w2(32bit register)
  // I used w1 and got no output when I using mailbox.
  // This is because that the w1 will destroy x1(upper 32 bits->0, lower load the content)
  ldr w2, =__bss_size
// Loop until all bss set to 0
2:
  cbz w2, 3f
  // xzr is 64 bits zero register, write all 0 to x1 then x1 = x1 + 8
  // https://blog.csdn.net/boildoctor/article/details/123359325
  str xzr, [x1], #8
  // sub w2 by 1 byte(8bits)
  sub w2, w2, #1
  cbnz w2, 2b

3:
  bl core_timer_enable
  // jump to C code, will not return in normal scenario
  bl main
  // this line is for safety to send cpu0 to busy waiting if not work properly
  b  halt

from_el2_to_el1:
  mov x0, (1 << 31) // EL1 uses aarch64
  msr hcr_el2, x0   // hcr: hypervisior configuration register. bit[31] is set indicate that the Execution state for EL1 is AArch64
  // 3C5->345 as unmask bit 7(IRQ mask), s.t. el1 IRQ interrupt is enabled
  mov x0, 0x345     // EL1h (SPSel = 1) with interrupt disabled.   spsel, which consists of a single bit that controls whether sp is an alias of sp_el1 or sp_el0
  msr spsr_el2, x0  // spsr: saved program state register   cpsr: current program state register
  msr elr_el2, lr   // Saved the current return address into elr_el2(exception link register, only for aarch64, used for returning to PC after exception)
  eret              // return to EL1. eret: reconstruct the processor state from current exception level's spsr_elx register and branch to the address in elr_elx

// save general registers to stack
.macro save_all
    sub sp, sp, 16 * 17
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
    // save information of interrupted task(used when nested interrupt occurs)
    mrs x0, spsr_el1
    stp x30, x0, [sp, 16 * 15]
    mrs x0, elr_el1
    // we need to store stack pointer in EL0 for Lab5
    mrs x1, sp_el0
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
    ldp x30, x0, [sp, 16 * 15]
    msr spsr_el1, x0
    // load information when interrupted task return(used when nested interrupt occurs)
    ldp x0, x1, [sp, 16 * 16]
    msr elr_el1, x0
    msr sp_el0, x1
    // restore x0 and x1(as we just use it to restore the information of spsr and elr)
    ldp x0, x1, [sp ,16 * 0]
    add sp, sp, 16 * 17
.endm

exception_handler:
  save_all
  bl c_exception_handler
  load_all
  eret

core_timer_enable:
  mov x0, 1
  msr cntp_ctl_el0, x0 // Timer enabled. Setting this bit to 0 disables the timer output signal, but the timer value accessible from CNTP_TVAL_EL0 continues to count down.
  mrs x0, cntfrq_el0    // This register is provided so that software can discover the frequency of the system counter. [31:0]: Clock frequency. Indicates the system counter clock frequency, in Hz.
  //mov x1, 2
  //mul x0, x0, x1
  msr cntp_tval_el0, x0 // set expired time
  mov x0, 2
  ldr x1, =CORE0_TIMER_IRQ_CTRL
  str w0, [x1] // unmask timer interrupt
  ret

core_timer_handler:
  //msr elr_el1, lr 
  //mrs x0, cntfrq_el0
  //mov x1, 2
  //mul x0, x0, x1
  //msr cntp_tval_el0, x0
  save_all
  bl c_core_timer_handler
  load_all
  eret

general_irq_handler:
  save_all
  mov x0, sp
  bl c_general_irq_handler
  //bl check_signal
  load_all
  eret

system_call_handler:
  save_all

  /*mrs	x25, esr_el1				// read the syndrome register
  lsr	x24, x25, #26   		// exception class, get bit[31:26]
  cmp	x24, #0x15			    // SVC in 64-bit state(https://blog.csdn.net/lsshao/article/details/119379653)
  b.ne	c_exception_handler*/

  mov x0, sp              // get trap frame(which is the same thing we just saved, meaning that we might modified user space registers)
  bl c_system_call_handler
  str x0, [sp, 0]         // save return value to trap frame
  //bl check_signal         // check if there is any signal
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
  b general_irq_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b system_call_handler // synchrounous exception from lower level
  .align 7
  b general_irq_handler
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


//_dtb_addr is in data section
.section .data
.global _dtb_addr
// make it 64bits
_dtb_addr: 
  .quad 0x0	