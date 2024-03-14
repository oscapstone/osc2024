.section ".text._start"

.global _start

_start:
    // read cpu id
    mrs     x0, MPIDR_EL1
    // mask off cpu id by two bits, 3(dec) = 11(bin)
    and     x0, x0, #3
    // cpu id > 0, skip to parking loop
    cbnz     x0, .L_parking_loop

    // Initialize DRAM.
	ldr	x0, __bss_start
	ldr x1, __bss_end_exclusive

.L_bbs_init_loop: 
    cmp	x0, x1
	b.eq	.L_prepare_rust
	stp	    xzr, xzr, [x0], #8
	b       .L_bbs_init_loop	

.L_prepare_rust:  
    // Set the stack pointer.
	ldr	x0, _start
	mov	sp, x0

	// Jump to Rust code.
    b       _start_rust

.L_parking_loop:  
    wfe
    b       .L_parking_loop

