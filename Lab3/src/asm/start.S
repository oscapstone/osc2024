#include "sys_regs.h"
.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	
//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0

.section ".text.boot"

.global _start
_start:
    ldr		x1, =_dtb_ptr	//put _dtb_ptr into register1
	str		x0, [x1]		//store dtb address from x0 to _dtb_ptr
    // read cpu id, stop slave cores
    mrs     x0, mpidr_el1
    and     x0, x0, 3
    cbnz    x0, hang_on
    
    //ldr     x0, =_start
    ldr     x0, =_stack_top
    mov     sp, x0

//from_el2_to_el1
el2_entry:
    //The Execution state for EL1 is AArch64
    ldr     x1, =HCR_EL2_VALUE      //(1 << 31) set EL1 aarch64 https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/HCR-EL2--Hypervisor-Configuration-Register
    msr     hcr_el2, x1     //hcr_el2 = x1 https://developer.arm.com/documentation/dui0068/b/ARM-Instruction-Reference/Miscellaneous-ARM-instructions/MSR

    
    ldr     x1, =SPSR_EL2_VALUE     // set processor restore state after calling eret  
    msr     spsr_el2, x1    //EL1 with SP_EL1 (EL1h) and mask DAIF
    //https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/SPSR-EL2--Saved-Program-Status-Register--EL2-
    //When the processor takes an exception to an AArch64 execution state, the PSTATE interrupt masks (PSTATE.DAIF) are set automatically. 
    //DAIF stands for debug, abort (SError), IRQ, and FIQ. The DAIF field is 4 bits, with each bit corresponding to one of the mentioned exception types. 
    //By writing a 1 to a bit in the field, we mask or ignore the exception type.

    // init sp for EL1
    mov     x0, 0x70000 
    msr     sp_el1, x0

    adr     x1, el1_entry           // eret return address
    msr     elr_el2, x1            //Exception Link Registers = x1

    eret     //Returns from an exception. It restores the processor state based on SPSR_ELn and branches to ELR_ELn, where n is the current exception level.
    //https://developer.arm.com/documentation/dui0802/b/A64-General-Instructions/ERET

el1_entry:
    /*mrs     x1, cpacr_el1      //x1 = cpacr_el1 https://developer.arm.com/documentation/ddi0595/2021-12/AArch64-Registers/CPACR-EL1--Architectural-Feature-Access-Control-Register
    //https://developer.arm.com/documentation/ddi0595/2021-12/AArch64-Registers/CPACR-EL1--Architectural-Feature-Access-Control-Register?lang=en#fieldset_0-21_20
    //set cpacr_el1[21:20] = 1 =>This control does not cause execution of any instructions to be trapped.
    orr     x1, x1, 0x300000        //or, turn on the accessibility to SIMD and floating registers 
    msr     cpacr_el1, x1  */

    ldr     x0, =exception_vector_table         // set up el1 exception table
    msr     vbar_el1, x0    //vbar_el1 = exception_vector_table

    // clear bss
    ldr     x1, =_bss_start
    ldr     w2, =_bss_size

clear_loop:
    cbz     w2, pseudo_main
    str     xzr, [x1], 8
    sub     w2, w2, 1
    cbnz    w2, clear_loop

pseudo_main:
    bl      main
    b       hang_on

hang_on:
    wfe
    b       hang_on

