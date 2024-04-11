// Run `svc 0` 5 times; svc (el0 -> el1) system calls:
// https://developer.arm.com/documentation/den0013/d/Exception-Handling/Other-exception-handlers/SVC-exception-handling
.section ".text"
.global _start
_start:
    mov x0, 0
1:
    add x0, x0, 1
    svc 0
    cmp x0, 5
    blt 1b
1:
    b 1b
