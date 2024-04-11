.section ".text.boot"

.global _start

_start:
    ldr		x21, =_dtb_ptr	//put _dtb_ptr into register1
	str		x0, [x21]		//store dtb address from x0 to _dtb_ptr
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1 // get cpu id
    and     x1, x1, #3 // mask off cpu id bits (cpu id is in bits 0-1)
    cbz     x1, set_stack  // cpu id == 0, continue
halt:  
    // cpu id > 0, loop
    wfe //(Wait for event status)
    b       halt // loop
set_stack:  
    // cpu id == 0
    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    
    bl      from_el2_to_el1             // switch to EL1 
    bl      set_exception_vector_table  // set up exception vector table 
    ldr     x1, =_start // get address of this code
    mov     sp, x1  // set stack pointer

    

    // clear bss
    ldr     x1, =__bss_start // get start of bss
    ldr     w2, =__bss_size // get size of bss
bss_loop:  
    // clear bss loop
    cbz     w2, run_main
    str     xzr, [x1], #8 // set 8 bytes to 0
    sub     w2, w2, #1
    cbnz    w2, bss_loop // loop
run_main:  
    // jump to C code, should not return
    bl      main
    // for failsafe, halt this core too
    b       halt

.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0


.global set_exception_vector_table
.global exception_vector_table
.global from_el2_to_el1


from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1


/* save general registers to stack */
.macro save_all
    sub sp, sp, 32 * 8
    stp x0, x1, [sp, 16 * 0]
    stp x2, x3, [sp, 16 * 1]
    stp x4, x5, [sp, 16 * 2]
    stp x6, x7, [sp, 16 * 3]
    stp x8, x9, [sp, 16 * 4]
    stp x10, x11, [sp, 16 * 5]
    stp x12, x13, [sp, 16 * 6]
    stp x14, x15, [sp, 16 * 7]
    stp x16, x17, [sp, 16 * 8]
    stp x18, x19, [sp, 16 * 9]
    stp x20, x21, [sp, 16 * 10]
    stp x22, x23, [sp, 16 * 11]
    stp x24, x25, [sp, 16 * 12]
    stp x26, x27, [sp, 16 * 13]
    stp x28, x29, [sp, 16 * 14]
    str x30, [sp, 16 * 15]
.endm


/* load general registers from stack */
.macro load_all
    ldp x0, x1, [sp, 16 * 0]
    ldp x2, x3, [sp, 16 * 1]
    ldp x4, x5, [sp, 16 * 2]
    ldp x6, x7, [sp, 16 * 3]
    ldp x8, x9, [sp, 16 * 4]
    ldp x10, x11, [sp, 16 * 5]
    ldp x12, x13, [sp, 16 * 6]
    ldp x14, x15, [sp, 16 * 7]
    ldp x16, x17, [sp, 16 * 8]
    ldp x18, x19, [sp, 16 * 9]
    ldp x20, x21, [sp, 16 * 10]
    ldp x22, x23, [sp, 16 * 11]
    ldp x24, x25, [sp, 16 * 12]
    ldp x26, x27, [sp, 16 * 13]
    ldp x28, x29, [sp, 16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 32 * 8
.endm


exception_handler:
    save_all
    bl exception_entry
    load_all
    eret

el1h_irq_handler:
    save_all
    bl el1h_irq_entry
    load_all
    eret

el0_irq_handler:
    save_all
    bl el0_irq_entry
    load_all
    eret

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    b exception_handler // branch to a handler function. /* Synchronous EL1t */
    .align 7 // entry size is 0x80, .align will pad 0
    // The exception handler for a synchronous 
    // exception from the current EL (EL1) using SP0.
    
    b exception_handler /* IRQ EL1t */
    .align 7
    // The exception handler for an IRQ exception
    // from the current EL(EL1) using SP0.

    b exception_handler /* FIQ EL1t */
    .align 7
    // The exception handler for an FIQ exception
    // from the current EL using SP0.

    b exception_handler /* Error EL1t */
    .align 7
    // The exception handler for a System Error 
    // exception from the current EL using SP0.

    b exception_handler /* Synchronous EL1h */
    .align 7
    b el1h_irq_handler  /* IRQ EL1h */
    .align 7
    b exception_handler /* FIQ EL1h */
    .align 7
    b exception_handler /* Error EL1h */
    .align 7

    b exception_handler /* Synchronous 64-bit EL0 */
    .align 7
    b el0_irq_handler   /* IRQ 64-bit EL0 */
    .align 7
    b exception_handler /* FIQ 64-bit EL0 */
    .align 7
    b exception_handler /* Error 64-bit EL0 */
    .align 7

    b exception_handler /* Synchronous 32-bit EL0 */
    .align 7
    b exception_handler /* IRQ 32-bit EL0 */
    .align 7
    b exception_handler /* FIQ 32-bit EL0 */
    .align 7
    b exception_handler /* Error 32-bit EL0 */
    .align 7

set_exception_vector_table:
    adr x0, exception_vector_table  // distance between pc here and exception_vector_table
    msr vbar_el1, x0
    ret
