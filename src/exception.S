.macro save_all			              // save general registers to stack
  sub sp, sp, 16 * 17
  stp x0, x1, [sp ,16 * 0]		    // store pair of registers to sp+16*0
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
  mrs x29, sp_el0                // sp_el0: user stack pointer for EL0
  stp x30, x29, [sp, 16 * 15]
  // for nested interrupt
	mrs x28, SPSR_EL1
	mrs x29, ELR_EL1
	stp x28, x29, [sp, 16 * 16]
.endm								            

.macro load_all
  // for nested interrupt
  ldp x0, x1, [sp, 16 * 16]
  msr SPSR_EL1, x0
	msr ELR_EL1,  x1
  ldp x30, x29, [sp, 16 * 15]
  msr sp_el0, x29
  ldp x28, x29, [sp ,16 * 14]
  ldp x26, x27, [sp ,16 * 13]
  ldp x24, x25, [sp ,16 * 12]
  ldp x22, x23, [sp ,16 * 11]
  ldp x20, x21, [sp ,16 * 10]
  ldp x18, x19, [sp ,16 * 9]
  ldp x16, x17, [sp ,16 * 8]
  ldp x14, x15, [sp ,16 * 7]
  ldp x12, x13, [sp ,16 * 6]
  ldp x10, x11, [sp ,16 * 5]
  ldp x8, x9, [sp ,16 * 4]
  ldp x6, x7, [sp ,16 * 3]
  ldp x4, x5, [sp ,16 * 2]
  ldp x2, x3, [sp ,16 * 1]
  ldp x0, x1, [sp ,16 * 0]		      //store pair of registers to sp+16*0
  add sp, sp, 16 * 17
.endm

.macro	ventry	label       //  used to create entries in the vector table.
	.align	7                 //  ('label' is a macro parameter) 
	b	\label
.endm
  
.global enable_interrupt    
enable_interrupt:           
  msr DAIFClr, 0xf          // DAIFCler: used to clear any or all of DAIF to 0 (modify PSTATE fileds directly)
  ret                          
                            // ARM processor state has 4 bits that are responsible for holding mask status for different types of interrupts.
                            // D: Masks debug exceptions(a special type of synchronous exceptions), A: Masks SErrors
                            // I: Masks IRQs, F: Masks FIQs
.global disable_interrupt
disable_interrupt:          // DAIFset: used to set any or all of DAIF to 1 (modify PSTATE fileds directly)
  msr DAIFSet, 0xf          
  ret

.global set_exception_vector_table			// use as function
set_exception_vector_table:
  adr x0, exception_vector_table
  msr vbar_el1, x0                // Vector Base Address Register for EL1: Holds the exception base address for any exception that is taken to EL1.
  ret                             // Bits[10:0] in the vbar reg is reserved, while bits[63:11] is the vector base address,
                                  // so the vector table address should be aligned to 0x800.

.align 11                       // vector table should be aligned to 0x800 (2^11)
.global exception_vector_table  // 16 exception handlers (4 exception levels multiplied by 4 execution states)
exception_vector_table:

  //Exception from the current EL while using SP_EL0
  ventry	sync_el1t			// Synchronous EL1t
  ventry	irq_el1t			// IRQ EL1t
  ventry	fiq_el1t			// FIQ EL1t
  ventry	error_el1t			// Error EL1t
  
  // Exception from the current EL while using SP_ELx
  ventry	sync_el1h			// Synchronous EL1h	
  ventry	irq_el1h		    // IRQ EL1h	
  ventry	fiq_el1h			// FIQ EL1h
  ventry	error_el1h			// Error EL1h

  // Exception from a lower EL and at least one lower EL is AArch64
  ventry	sync_el0_64			// Synchronous 64-bit EL0
  ventry	irq_el0_64			// IRQ 64-bit EL0
  ventry	fiq_el0_64			// FIQ 64-bit EL0
  ventry	error_el0_64		// Error 64-bit EL0

  //Exception from a lower EL and at least all lower EL are AArch32
  ventry	sync_el0_32			// Synchronous 32-bit EL0
  ventry	irq_el0_32			// IRQ 32-bit EL0
  ventry	fiq_el0_32			// FIQ 32-bit EL0
  ventry	error_el0_32		// Error 32-bit EL0


sync_el1t:
  save_all
  bl  invaild_exception_handler
  load_all
  eret
irq_el1t:
  save_all
  bl  invaild_exception_handler
  load_all
  eret

fiq_el1t:
  save_all
  bl  invaild_exception_handler
  load_all
  eret

error_el1t:
  save_all
  bl  invaild_exception_handler
  load_all
  eret

sync_el1h:
  save_all
  bl  invaild_exception_handler
  load_all
  eret

irq_el1h:
  save_all
  bl  irq_interrupt_handler
  load_all
  eret

fiq_el1h:
  save_all
  bl  invaild_exception_handler
  load_all
  eret

error_el1h:
  save_all
  bl  invaild_exception_handler
  load_all
  eret

sync_el0_64:	
  save_all
  mov x0, sp // for trapframe
  bl  el0_64_sync_interrupt_handler
  load_all
  eret
irq_el0_64:
  save_all
  bl  irq_interrupt_handler
  load_all
  eret
fiq_el0_64:
  save_all
  bl  invaild_exception_handler
  load_all
  eret			
error_el0_64:		
  save_all
  bl  invaild_exception_handler
  load_all
  eret
sync_el0_32:			
  save_all
  bl  invaild_exception_handler
  load_all
  eret
irq_el0_32:			
  save_all
  bl  invaild_exception_handler
  load_all
  eret
fiq_el0_32:		
  save_all
  bl  invaild_exception_handler
  load_all
  eret
error_el0_32:		
  save_all
  bl  invaild_exception_handler
  load_all
  eret


  

