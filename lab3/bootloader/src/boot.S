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
.section ".text._relocate"

.global _start

_relocate:
_start:
	mrs	x1, mpidr_el1		
	and	x1, x1,#0xFF		// Check processor id
	cbz	x1, master		// Hang for all non-primary CPU
	b	proc_hang

proc_hang: 
	wfe
	b 	proc_hang

master:
setup_stack:
    ldr     x1, =_stack_top
    mov     sp, x1

setup_bss:
    ldr     x1, =_bss_top
    ldr     w2, =_bss_size

init_bss:
    cbz     w2, run_main
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, init_bss

store_dtb:
    mov     x10, x0
    //str     x0, [x1]

    b relocate

.section ".text.boot"

setup_bootloader_stack:
    ldr     x1, =_bootloader_stack_top
    mov     sp, x1

setup_bootloader_bss:
    ldr     x1, =_bootloader_bss_top
    ldr     w2, =_bss_size

init_bootloader_bss:
    cbz     w2, run_main
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, init_bootloader_bss

run_main:
    mov    x0, x10
    b      main

