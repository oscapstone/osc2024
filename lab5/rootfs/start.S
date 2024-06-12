.section ".text.boot"

// for application to start

.globl _start
_start:
    bl      main

    // exit syscall, and main retrun store in x0
    mov     x8, 5
    svc     0
