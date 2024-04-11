.section ".text.kernel"

.global _start

_start:
	ldr		x1, =_dtb_ptr	//put _dtb_ptr into register1
	str		x0, [x1]		//store dtb address from x0 to _dtb_ptr
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, setting
    // cpu id > 0, stop

halt:  wfe
    b       halt

setting:  // cpu id == 0

    ldr     x1, =_start
    bl		from_el2_to_el1
	mov     sp, x1

    
	/* set exception vector table */
    adr		x0, exception_vector_table  //x0 = exception_vector_table 的 address https://developer.arm.com/documentation/dui0041/c/Babcjaii
    msr		vbar_el1, x0    // Load Vector Base Address Register from the contents of x0 https://developer.arm.com/documentation/dui0068/b/ARM-Instruction-Reference/Miscellaneous-ARM-instructions/MSR
                            //從x0取出EL1的vector base address

	//bl		core_timer_enable
 	
	// clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

clear_bss:  
    cbz     w2, kernel
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, clear_bss
	//bl		core_timer_enable

	// jump to C code, should not return
kernel:  
    bl      main
    // for failsafe, halt this core too
    b       halt

from_el2_to_el1:
    // EL1 uses aarch64
	mov		x0,#(1<<31) //in order to let hcr_el2[31] = 1
	msr		hcr_el2,x0  //The Execution state for EL1 is AArch64. The Execution state for EL0 is determined by the current value of PSTATE.nRW when executing at EL0.
                        //https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/HCR-EL2--Hypervisor-Configuration-Register?lang=en#fieldset_0-31_31-1
	mov		x0,#0x3c5 //EL1h (SPSel = 1) with interrupt disabled
    //0x345 = 1101000101    0x3c5 = 1111000101
    //spsr_el2[3:0] = 0101 => EL1 with SP_EL1 (EL1h) 使用EL1的stack pointer
    //https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/SPSR-EL2--Saved-Program-Status-Register--EL2-
    //When the processor takes an exception to an AArch64 execution state, the PSTATE interrupt masks (PSTATE.DAIF) are set automatically. 
    //DAIF stands for debug, abort (SError), IRQ, and FIQ. The DAIF field is 4 bits, with each bit corresponding to one of the mentioned exception types. 
    //By writing a 1 to a bit in the field, we mask or ignore the exception type.
    //https://developer.arm.com/documentation/102412/0103/Handling-exceptions/Exception-handling-examples?lang=en
	msr		spsr_el2,x0 //spsr_el2[7] => IRQ interrupt mask 1 = interrupt disabled
	
    mov     x0, 0x70000
    msr     sp_el1, x0
    msr		elr_el2,lr  //Exception Link Registers = lr
	eret    //Returns from an exception. It restores the processor state based on SPSR_ELn and branches to ELR_ELn, where n is the current exception level.
    //https://developer.arm.com/documentation/dui0802/b/A64-General-Instructions/ERET
/*
core_timer_enable:
	//mov	x0,1
	//msr cntp_ctl_el0, x0 //enable
	//mrs x0, cntfrq_el0
	//mov x3,5
	//mul x0, x0, x3
	//msr cntp_tval_el0,x0 // set expired time
	//mov x0,2
	//ldr x1, =CORE0_TIMER_IRQ_CTRL
	//str w0,[x1] //unmask timer interrupt
	//ret
*/
// save general registers to stack
//https://software-dl.ti.com/codegen/docs/tiarmclang/rel1_3_0_LTS/gnu_syntax_arm_asm_language/gnu_arm_macros.html
//It defines a name that you can use as a convenient alternative to repeating the block of code.
.macro save_all
    sub sp, sp, 32 * 8  //32 register, 1 register 8 byte
    stp x0, x1, [sp ,16 * 0]    //store x0, x1 at sp+16*0
    stp x2, x3, [sp ,16 * 1]   //store x2, x3 at sp+16*1
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
.macro load_all
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



exception_handler:
    save_all
    bl except_handler_c
    load_all
    eret

irq_exception_handler:
	save_all
	bl irq_except_handler_c
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
  b irq_exception_handler
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

.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0

