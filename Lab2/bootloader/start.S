.section ".text.boot"

.global _start	

/*https://github.com/bztsrc/raspi3-tutorial/blob/master/14_raspbootin64/start.S
 save arguments in registers (we will need them later for the new kernel)
 I choosed x10-x13 because instructions generated from C by gcc does not
 touch them. You can check that with "aarch64-elf-objdump -d kernel8.elf" */

_start:
	mov x10, x0 //store dtb address
	// relocate bootloader
	ldr x1, =0x80000 /*x1 store data of 0x80000 */
	ldr x2, =__bootloader_start /*x2 store data of 0x60000 */
	ldr w3, =__bootloader_size	/*get bootloader size */

// relocate bootloader
relocate:
	ldr x4,[x1],#8  /* get the data from x1(0x80000) to x4 and +8 */
	str x4,[x2],#8	/* put the data in x2(0x60000) from x4 and +8 */
	sub w3,w3,#1	/* bootloader size -1 */
	cbnz w3,relocate	/* bootloader size !=0, go relocate */

/*set top of stack just before our code
set up sp*/
setting: 
	ldr x1, =_start
 	mov sp, x1
	ldr x1, =__bss_start
	ldr w2, =__bss_size

clear_bss: 
	cbz w2, bootloader_main	/* bss size = 0, go bootloader_main */
	str xzr,[x1],#8
	sub w2, w2, #1
	cbnz w2, clear_bss	/* bss size != 0, go clear_bss */

bootloader_main: 
	bl main-0x20000	/* go to 0x60000 (bootloader) */
	b  bootloader_main
