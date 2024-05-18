// for application to start

.globl _start:
_start:
    bl      main

    // exit syscall
    mov     x8, 5
    svc     0
