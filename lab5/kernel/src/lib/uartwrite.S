

.globl uartwrite
uartwrite:
    mov     x8, 2
    svc     0
    ret
