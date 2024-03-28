.section ".text.boot"

.global _start

_start:
	// store the address of the dtb
	mov x24, x0

	// relocate bootloader
	ldr x1, =0x80000 
	ldr x2, =__bootloader_start //x11=0x60000
	ldr w3, =__bootloader_size

relocate:
	ldr x4,[x1],#8 //store value from x1 to x4 than mov x1
	str x4,[x2],#8 //store value from x2 to x4 than mov x2
	sub w3,w3,#1
	cbnz w3,relocate

set_stack: 
	ldr x1, =_start
 	mov sp, x1
	ldr x1, =__bss_start
	ldr w2, =__bss_size

bss_loop: 
	cbz w2, bootloader_main
	str xzr,[x1],#8
	sub w2, w2, #1
	cbnz w2, bss_loop

bootloader_main: 
	bl main-0x20000
	b  bootloader_main
