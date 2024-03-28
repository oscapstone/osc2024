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

// x0 is used for dtb physical address
.section ".text.boot"

.global _start

_start:
setup_stack:
    ldr     x1, =_stack_top     // Load address of the stack top into x1
    mov     sp, x1              // Set the stack pointer to the top of the stack

setup_bss:
    ldr     x1, =_bss_top       // Load address of the start of the BSS section into x1
    ldr     w2, =_bss_size      // Load size of the BSS section into w2

init_bss:
    cbz     w2, run_main        // If BSS size is zero, jump to run_main
    str     xzr, [x1], #8       // Store zero (from register xzr) to address in x1, then increment x1 by 8
    sub     w2, w2, #1          // Decrement BSS size counter
    cbnz    w2, init_bss        // If BSS size counter is not zero, loop back to init_bss

run_main:
    ldr     x1, =_dtb           // Load address of the Device Tree Blob (DTB) into x1
    str     x0, [x1], #8        // Store the value in x0 (DTB address passed by bootloader) to the address in x1, then increment x1 by 8
    bl      main                // Branch with link to the main function

proc_hang:
    wfe                         // Wait for event
    b       proc_hang           // Loop indefinitely (hang)