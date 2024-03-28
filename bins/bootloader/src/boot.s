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
	adrp  \register, \symbol
	add \register, \register, #:lo12:\symbol
.endm

// Load the address of a symbol into a register, absolute.
//
// # Resources
//
// - https://sourceware.org/binutils/docs-2.36/as/AArch64_002dRelocations.html
.macro ADR_ABS register, symbol
  movz  \register, #:abs_g2:\symbol
  movk  \register, #:abs_g1_nc:\symbol
  movk  \register, #:abs_g0_nc:\symbol
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
  // We need to initialize the bss which is relocated,
  // so we have to load absolute address
	ADR_ABS	x1, __bss_begin
	ADR_ABS x2, __bss_end

.L_bss_init_loop:
	cmp	x1, x2
	b.eq	.L_relocate_self
	stp	xzr, xzr, [x1], #16
	b	.L_bss_init_loop

.L_relocate_self:
  ADR_REL x1, __binary_nonzero_start // load the current running binary start address
  ADR_ABS x2, __binary_nonzero_start // load the actually start address that the bootloader will be relocated to
  ADR_ABS x3, __binary_nonzero_end

.L_copy_loop:
  ldr x4, [x1], #8
  str x4, [x2], #8
  cmp x2, x3
  b.lo .L_copy_loop

	// Prepare the jump to Rust code.
.L_prepare_rust:
	// Set the stack pointer.
	ADR_ABS	x1, __binary_nonzero_start
	mov	sp, x1

  ADR_ABS x1, _start_rust
	// Jump to Rust code.
	br x1

	// Infinitely wait for events (aka "park the core").
.L_parking_loop:
	wfe
	b	.L_parking_loop

.size	_start, . - _start
.type	_start, function
.global	_start
