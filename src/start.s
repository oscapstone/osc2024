.section ".text.boot"

.global _start

_start:
    mrs     x1, MPIDR_EL1   // get cpu id and put to reg x1
    and     x1, x1, #3      // Keep the lowest two bits
    cbz     x1, 2f          // if cpu_id > 0, stop
1:
    wfe                     // if cpu_id == 0
    b       1b
2:
    ldr     x1, =_start     // set stack pointer
    mov     sp, x1
    ldr     x1, =__bss_start    // clear bss
    ldr     x2, =__bss_size
3:  
    cbz     x2, 4f          // if val in reg x2 is zero, thn jump to label 4, indicating that the .bss section has been zeroed.
    str     xzr, [x1], #8   // write a zero value (from the xzr register) to the memory address pointed to by x1, and then increment the value of x1, effectively zeroing the next 8 bytes.
    sub     x2, x2, #1
    cbnz    x2, 3b
4:  
    bl      main            // main function
    b       1b              // halt this core if return
