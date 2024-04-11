.section ".text"
.global _start
_start:
    mov x0, 0
1:
    add x0, x0, 1
    // it can be retrieved by the exception handler to determine what service is being requested.
    svc 0   //https://developer.arm.com/documentation/dui0489/i/arm-and-thumb-instructions/svc
    cmp x0, 5
    blt 1b
1:
    //svc 4
    b 1b