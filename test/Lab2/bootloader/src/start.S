.section ".text.boot"

.global _start

_start:
	mov     x10, x0 // dtb address
    mov     x11, x1 /* x1-x3 must is 0 */
    mov     x12, x2
    mov     x13, x3
    // get data
	ldr x1, =0x80000 //x1 store data of 0x80000
	ldr x2, =_loader_start //x11 store data of 0x60000 
	ldr w3, =_loader_size //get bootloader size

//relocate bootloader
relocate:
	ldr x4,[x1],#8  // get the data from x1(0x80000) and +8
	str x4,[x2],#8 // put the data in x2(0x60000) and +8
	sub w3,w3,#1 // bootloader size -1
	cbnz w3,relocate // bootloader size !=0, go relocate 

//set up sp
setting: 
	ldr x1, =_start
 	mov sp, x1
	ldr x1, =_bss_start
	ldr w2, =_bss_size

//clear bss
clear_bss: 
	cbz w2, bootloader_main // bss size = 0, go bootloader_main
	str xzr,[x1],#8
	sub w2, w2, #1
	cbnz w2, clear_bss // bss size != 0, go clear_bss

bootloader_main: 
	bl main-0x20000 // go to 0x60000 (bootloader)
	b  bootloader_main