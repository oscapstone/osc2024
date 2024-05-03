    .section ".text"
    .global _start
_start:
    mov     x0, 0
1:
    add     x0, x0, 1
    svc     0       // system call (el0 -> el1) 
    cmp     x0, 5   // Run `svc 0` 5 times
    blt     1b
1:
    b       1b