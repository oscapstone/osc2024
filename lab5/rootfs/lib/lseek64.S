

.globl lseek64
lseek64:
    mov     x8, 18
    svc     0
    ret
