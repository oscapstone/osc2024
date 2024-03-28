.section ".text.boot"

.global _start

_start:
    mrs     x1, MPIDR_EL1   // get cpu id and put to reg x1
    and     x1, x1, #3      // Keep the lowest two bits
    cbz     x1, relocate_setting         // if cpu_id > 0, stop
proc_hang:
    wfe                     
    b       proc_hang

relocate_setting: 
	ldr		x1, =_dtb_ptr	 // put _dtb_ptr into register1
	str		x0, [x1]		// store dtb address from x0 to _dtb_ptr
	ldr x1, =0x80000 
	ldr x2, =__bootloader_start 
	ldr w3, =__bootloader_size

relocate:
	ldr 	x4,	[x1], #8 
	str 	x4,	[x2], #8 
	sub 	w3,	w3, #1
	cbnz 	w3,	relocate
	
setting: 
	ldr x1, =_start
 	mov sp, x1
	ldr x1, =__bss_start
	ldr w2, =__bss_size

clear_bss: 
	cbz w2, bootloader_main
	str xzr,[x1],#8
	sub w2, w2, #1
	cbnz w2, clear_bss
	
bootloader_main: 
	bl main
	b  proc_hang

.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0