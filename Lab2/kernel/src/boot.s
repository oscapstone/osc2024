.section ".text.boot"

.global _start

_start:

    // dtb
    ldr     	x1, =0x200000
    str     	x0, [x1] 

    /* read cpu id, stop slave cores */
    mrs         x1, mpidr_el1
    and         x1, x1, #3
    cbz         x1, set_stack 
    /* cpu id > 0, stop */

proc_hang:
    /* cpu id > 0 */
    wfe
	b 		    proc_hang 

set_stack:
    ldr         x1, =_start
    mov         sp, x1
    ldr         x1, =__bss_start
    ldr         w2, =__bss_size

clear_bss:
    cbz         w2, run_main
    str         xzr, [x1], #8
    sub         w2, w2, #1
    cbnz        w2, clear_bss

run_main:
    bl          main
	b 		    proc_hang
