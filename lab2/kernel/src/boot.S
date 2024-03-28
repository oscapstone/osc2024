/* ARMv8 Assembly Instruction */
/**

mov x0, x1
    sets: x0 = x1
ldr x0, <addr>
    load 32bits from <addr> to x0
ldr w0, <addr>
    load 64bits from <addr> to w0
cbz x0, <label>
    if x0 == 0, jump to <label>
cbnz x0, <label>
    if x0 != 0, jump to <label>
str x0 [x1] #8
    store x0 in addr<x1> then x1=x1+8
b   <label>
    jump to <label>
bl  <label>
    jump to <label> and copies bl's next instruction into link register
wfe
    Wait for event, core in low-power state (power on, clk off)

**/

.section ".text.boot"   // Define a section for the boot code

.global _start          // Make the _start symbol globally visible

_start:                 // Entry point of the bootloader
setup_stack:
    ldr     x1, =_stack_top  // Load the address of the stack top into x1
    mov     sp, x1           // Set the stack pointer (sp) to the stack top

setup_bss:
    ldr     x1, =_bss_top    // Load the address of the beginning of the BSS section into x1
    ldr     w2, =_bss_size   // Load the size of the BSS section into w2

init_bss:
    cbz     w2, run_main     // If w2 (BSS size) is zero, jump to run_main
    str     xzr, [x1], #8    // Store zero (from the zero register xzr) to the address in x1, then increment x1 by 8 bytes
    sub     w2, w2, #1       // Decrement the BSS size counter (w2) by 1
    cbnz    w2, init_bss     // If w2 is not zero, loop back to init_bss

run_main:
    bl      main             // Branch with link to the main function (calling main)

proc_hang:
    wfe                      // Wait for event (puts the processor in a low-power state until an event occurs)
    b       proc_hang        // Branch to proc_hang (infinite loop to hang the processor)

