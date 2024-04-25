.section ".text.boot"

.global _start

_start:
    ldr     x1, =_dtb_ptr
    str     x0, [x1]
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3  //Check processor id
    cbz     x1, 2f      //if CPU id = 0, jump to 2f
    b       1f

1:  wfe
    b       1b

2:  ldr     x1, =_start
    mov     sp, x1
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

3:  cbz     w2, 4f
    str     xzr, [x1], #8  // x1* = xzr, x1 += 8 (write xzr represents discard, read xzr == 0)
    sub     w2, w2, #1
    cbnz    w2, 3b

4:  bl      main
    b       1b


.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0