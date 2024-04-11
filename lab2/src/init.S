.section ".text.boot"

.globl _start
_start:                         // entry point -> to work with only the first one cpu and put all of the other cores in an endless loop.
    mrs     x2, mpidr_el1       // get process id from mpidr_el1 register to x2
    and     x2, x2, #0xFF       // Check processor id
    cbz     x2, primary         // if x2 == 0, jump to master, if not equal to 0, means non-primary processor
    b       non_primary

non_primary:                    // if non-primary processor, to endless loop. -> easier, avoid racing...
    b       non_primary

primary:                        // set bss section to zero
    adr     x2, bss_begin       // generates a PC-relative address of start point of bss section
    adr     x1, bss_end         // generates a PC-relative address of end point of bss section
    sub     x1, x1, x2          // save x1-x2 to x1 -> len of bss section

memzero:                        // initialize memory section to zero, need start point and length
	str     xzr, [x2], #8       // save 0(xzr is zero register) to x2 address and let x2 = x2+8
	subs    x1, x1, #8          // let x1 = x1-8, represent there are 8 bytes addressed
	b.gt    memzero             // if x1 > 0, jump to memzero
    b       final

final:
    ldr     x1, =_start         // load address of _start
    mov     sp, x1              // inital stack pointer to _start, because stack is grow down
    bl      kernel_main         // then jump to kernel_main function
