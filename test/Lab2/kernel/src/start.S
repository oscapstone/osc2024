.section ".text.boot"

.global _start

_start:
    ldr		x1, =_dtb_ptr	//put _dtb_ptr into register1
	str		x0, [x1]		//store dtb address from x0 to _dtb_ptr
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, setting
    // cpu id > 0, stop
halt:  wfe
    b       halt
setting:  // cpu id == 0

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    adr     x1, _start
    mov     sp, x1

    // clear bss
    adr     x1, __bss_start
    ldr     w2, =__bss_size
clear_bss:  
    cbz     w2, kernel
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, clear_bss

    // jump to C code, should not return
kernel:  
    bl      main
    // for failsafe, halt this core too
    b       halt

.section .data		//_dtb_ptr is in data section
.global _dtb_ptr	//define a global variable _dtb_ptr
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0