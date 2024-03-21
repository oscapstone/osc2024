.section .text._start

_stack_start    = 0x70000

.global _start
.global _bss_start
.global _bss_end

_start:
    mrs x0, mpidr_el1
    cbz x0, .L_parking_loop

    # Initailize stack pointer
    ldr x0, =_stack_start
    mov sp, x0
    
    # Initialize bss
   ldr x0, =_bss_start
   ldr x1, =_bss_end
.L_clear_bss:
   cmp x0, x1
   bge .L_done_clearing
   str xzr, [x0], #8
   b .L_clear_bss
.L_done_clearing:

    # Call rust main function
    b   _start_rust

.L_parking_loop:
    wfe
    b   .L_parking_loop

.size	_start, . - _start
.type	_start, function
.global	_start