
.macro ADR_REL register, symbol
	adrp	\register, \symbol
	add	\register, \register, #:lo12:\symbol
.endm

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------
.section .text._start

//------------------------------------------------------------------------------
// fn _start()
//------------------------------------------------------------------------------
_start:
	// Only proceed on the boot core. Park it otherwise.
	mrs	x0, MPIDR_EL1
	and	x0, x0, {CONST_CORE_ID_MASK}
	ldr	x1, BOOT_CORE_ID      // provided by bsp/__board_name__/cpu.rs
	cmp	x0, x1
	b.ne	.L_parking_loop
	
	// If execution reaches here, it is the boot core.

	// Initialize DRAM.
	ADR_REL	x0, __bss_start
    ADR_REL x1, __bss_end_exclusive

.L_bss_init_loop:
 	cmp	    x0, x1
 	b.eq	  .L_prepare_rust
  	stp     xzr, xzr, [x0], #16
  	b       .L_bss_init_loop
  

	// Prepare the jump to Rust code.
.L_prepare_rust:
	// Set the stack pointer.
  	mov sp, 0x60000

	// Jump to Rust code.
	bl	_start_rust

	// jump to 0x80000
	ldr x0, =0x80000	
	br x0

	// Infinitely wait for events (aka "park the core").
.L_parking_loop:
	wfe
	b	.L_parking_loop

.size	_start, . - _start
.type	_start, function
.global	_start
