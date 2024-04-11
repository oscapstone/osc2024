.section ".text.boot"

.globl _start
_start:                             // entry point -> to work with only the first one cpu and put all of the other cores in an endless loop.
    mov     x10, x0

    mrs     x0, mpidr_el1       	// get process id from mpidr_el1 register to x0
    and     x0, x0, #0xFF       	// Check processor id
    cbz     x0, bootloader         	// if x0 == 0, jump to master, if not equal to 0, means non-primary processor
    b       non_primary

non_primary:                    	// if non-primary processor, to endless loop. -> easier, avoid racing...
    wfe
    b       non_primary

bootloader:
	ldr     x1, =0x80000           	// loads the address 0x80000 into x1.
	ldr     x2, =bootloader_begin  	// loads the address stored in bootloader_begin(0x60000) into x2.
	ldr     x3, =bootloader_size

relocate:
	ldr     x4, [x1], #8           	// loads a 64-bit value from the memory address pointed to by x0 into x4, and then increments x0 by 8 bytes.
	str     x4, [x2], #8           	// stores the value in x4 into the memory address pointed to by x1, and then increments x1 by 8 bytes.
	sub     x3, x3, #1              // w3 = w3 - 1
	cbnz    x3, relocate            // if w3 != 0, jump to relocate section

clear_bss:                          // set bss section to zero
    adr     x0, bss_begin           // generates a PC-relative address of start point of bss section
    adr     x1, bss_end             // generates a PC-relative address of end point of bss section
    sub     x1, x1, x0              // save x1-x0 to x1 -> len of bss section

memzero:                            // initialize memory section to zero, need start point and length
	str     xzr, [x0], #8           // save 0(xzr is zero register) to x0 address and let x0 = x0+8
	subs    x1, x1, #8              // let x1 = x1-8, represent there are 8 bytes addressed
	b.gt    memzero                 // if x1 > 0, jump to memzero
    b       set_sp

set_sp:
    ldr     x1, =_start             // load address of _start
    mov     sp, x1                  // inital stack pointer to _start, because stack is grow down

final:
    bl      bootloader_main-0x20000 // then jump to bootloader_main function
    b       final

