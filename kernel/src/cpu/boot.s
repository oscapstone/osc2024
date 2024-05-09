.section .text._start

_stack_start    = 0x70000

.global _start
.global _bss_start
.global _bss_end

.macro ADR_REL register, symbol
  adrp  \register, \symbol
  add  \register, \register, #:lo12:\symbol
.endm

_start:
    # Store the address of the device tree
    # Should be removed when building for a real hardware
    # ldr x1, =0x75100
    # str x0, [x1]

    mrs x0, mpidr_el1
    cbz x0, .L_parking_loop

    # Initailize stack pointer
    ldr x0, =_stack_start
    mov sp, x0

    ldr x0, =0x75000
    str xzr, [x0]
    
    # Initialize bss
    ldr x0, =_bss_start
    ldr x1, =_bss_end
.L_clear_bss:
   cmp x0, x1
   bge .L_done_clearing
   str xzr, [x0], #8
   b .L_clear_bss
.L_done_clearing:

    bl .from_el2_to_el1

    mrs x0, CurrentEL
    cmp x0, #0x4 // Check if we are in EL1
    bne .from_el2_to_el1

.set_exception_vector_table:
    adr x0, exception_vector_table
    msr vbar_el1, x0

    # Call rust main function
    b   _start_rust

.from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) witH interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    mov x0, sp
    msr sp_el1, x0
    eret // return to EL1

.L_parking_loop:
    wfe
    b   .L_parking_loop

.size	_start, . - _start
.type	_start, function


// .section .text.exception_vector_table
.align 11
.global exception_vector_table
exception_vector_table:
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
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

