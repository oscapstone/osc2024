
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
	ADR_REL x1, __dtb_addr
	str x0, [x1]
	// Only proceed on the boot core. Park it otherwise.
	mrs	x0, MPIDR_EL1
	and	x0, x0, {CONST_CORE_ID_MASK}
	ldr	x1, BOOT_CORE_ID      // provided by bsp/__board_name__/cpu.rs
	cmp	x0, x1
	b.ne	.L_parking_loop

	// get addresses
	ldr x0, =0x60000
	ADR_REL x1, __relo_start
	ADR_REL x2, __relo_end

.L_relo_loop:
	cmp x1, x2
	b.eq .L_relo_end
	// load data from x1
	ldr x3, [x1]
	// store into [x0]
	str x3, [x0]
	// move to next address
	add x1, x1, #8
	add x0, x0, #8
	b .L_relo_loop

.L_relo_end:
	ldr x0, =.L_no_relo
	br x0

.L_no_relo:
	// Initialize DRAM.
	ADR_REL	x0, __bss_start
    ADR_REL x1, __bss_end_exclusive

.L_bss_init_loop:
 	cmp	    x0, x1
 	b.eq	.L_prepare_rust
  	stp     xzr, xzr, [x0], #16
  	b       .L_bss_init_loop


	// Prepare the jump to Rust code.
.L_prepare_rust:
	// Set the stack pointer.
  	mov sp, 0x60000

	// Jump to Rust code.
	bl	_start_rust

	// load __dtb_addr value
	ADR_REL x1, __dtb_addr
	ldr x0, [x1]
	
	// jump to 0x80000
	ldr x1, =0x80000
	br x1

	// Infinitely wait for events (aka "park the core").
.L_parking_loop:
	wfe
	b	.L_parking_loop

.size	_start, . - _start
.type	_start, function
.global	_start
