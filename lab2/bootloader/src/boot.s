.section ".text.boot"

.global _start

_start:
	//refer to raspberry pi aarch64 booting:
	//https://github.com/raspberrypi/linux/blob/8c3e7a55220cb7cb13131bb8dccd37694537eb97/Documentation/arch/arm64/booting.rst#L459
	

	//x0 = physical address of device tree blob (dtb) in system RAM.
	//x1 = 0 (reserved for future use)
	//x2 = 0 (reserved for future use)
	//x3 = 0 (reserved for future use)

	//save x0 - x3 regs to x10 - x13, and move them back after image loading process.
	mov     x10, x0  
    mov     x11, x1  
    mov     x12, x2
    mov     x13, x3
	
	// relocate bootloader
	//= means immediate, so that we save the address in the regs
	ldr x1, =0x80000 
	ldr x2, =__bootloader_start //0x60000
	ldr w3, =__bootloader_size

relocate:
	// start to get bootloader data from 0x80000, and then store it to 0x60000, 0x60008...etc.
	ldr x4,[x1],#8  
	str x4,[x2],#8  
	sub w3,w3,#1
	cbnz w3,relocate

cleanbss: 
	ldr x1, =_start
 	mov sp, x1
	ldr x1, =__bss_start
	ldr w2, =__bss_size

memzero: 
	cbz  w2, bootloader_main
	str  xzr,[x1],#8
	sub  w2, w2, #1
	cbnz w2, memzero

bootloader_main: 
	// since text.boot. is now start from 0x60000, kernel_main new address is also subtracted by 0x20000
	bl kernel_main-0x20000 //this kernel is bootloader
	b  bootloader_main

 
