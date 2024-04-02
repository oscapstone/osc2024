.section ".text.boot"

.global _start

_start:
	ldr		x1, =_dtb_ptr	//put _dtb_ptr into register1(store a address in x1)
	str		x0, [x1]		//store dtb address from x0 to _dtb_ptr
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, cleanbss
    // cpu id > 0, stop

proc_hang:  
    wfe
    b       proc_hang

cleanbss:  
    // cpu id == 0
    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start
    mov     sp, x1
    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

memzero:
    cbz     w2, kernel_main
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, memzero

    // jump to C code, should not return
kernel_main:  
    bl      main
    // for failsafe, halt this core too
    b       kernel_main

.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
// define constant address
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0
