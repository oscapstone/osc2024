.section ".text.boot"

.global _start

_start:
    ldr     x0, =_start
    mov     sp, x0
    ldr     x0, =__bss_start
    ldr     x1, =__bss_size
    bl 	memzero
    cbz     x1, run_main
    
run_main:
    bl      main
proc_hang:
    wfe
    b       proc_hang    
    
memzero:
	str xzr, [x0], #8
	subs x1, x1, #1
	b.gt memzero
	ret
