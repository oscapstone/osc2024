

.globl ioctl
ioctl:
    mov     x8, 19
    svc     0
    ret
