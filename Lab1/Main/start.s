.section ".text.boot"

.globl _start
_start:
    mrs    x0, mpidr_el1        
    and    x0, x0, #0xFF      // Check processor id
    cbz    x0, master         // Hang for all non-primary CPU

master:
    adr    x0, bss_begin
    adr    x1, bss_end
    sub    x1, x1, x0
    bl     clear_bss

clear_bss:
 	cbz  	x1, prepare_kernel
	str  	xzr, [x0], #8
	subs 	x1, x1, #8
	b    	clear_bss
	
prepare_kernel:
    mov    sp, #0x3F000000
    bl     kernel_main
