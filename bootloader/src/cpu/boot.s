.section .text._start

_stack_start    = 0x90000

.global _start
.global _text_size
.global _bss_start
.global _bss_end

_start:
    # Copy code to addres 0x60000
    ldr x0, =0x80000
    ldr x1, =0x60000
    ldr x2, =_text_size

.L_copy_loop:
    ldr w3, [x0], #4
    str w3, [x1], #4
    sub x2, x2, #4
    # check if we are done
    cbnz x2, .L_copy_loop

    ldr x0, =_boot
    br x0

_boot:
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