

.globl mbox_call
mbox_call:
    mov     x8, 6
    svc     0
    ret
