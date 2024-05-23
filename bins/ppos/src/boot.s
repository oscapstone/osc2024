// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2021-2022 Andre Richter <andre.o.richter@gmail.com>

//--------------------------------------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------------------------------------

// Load the address of a symbol into a register, PC-relative.
//
// The symbol must lie within +/- 4 GiB of the Program Counter.
//
// # Resources
//
// - https://sourceware.org/binutils/docs-2.36/as/AArch64_002dRelocations.html
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
	mrs	x1, MPIDR_EL1
	and	x1, x1, #3
	mov	x2, #0
	cmp	x1, x2
	b.ne	.L_parking_loop

	// If execution reaches here, it is the boot core.

	// Initialize DRAM.
	ADR_REL	x1, __bss_begin
	ADR_REL x2, __bss_end

.L_bss_init_loop:
	cmp	x1, x2
	b.eq	.L_prepare_rust
	stp	xzr, xzr, [x1], #16
	b	.L_bss_init_loop

	// Prepare the jump to Rust code.
.L_prepare_rust:
	// Set the stack pointer.
	mov	x1, 0x80000
	mov	sp, x1

	// Jump to Rust code.
	b _start_rust

	// Infinitely wait for events (aka "park the core").
.L_parking_loop:
	wfe
	b	.L_parking_loop

.size	_start, . - _start
.type	_start, function
.global	_start
