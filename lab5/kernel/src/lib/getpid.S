

.globl getpid
getpid:
    mov     x8, 0
    svc     0
    ret
