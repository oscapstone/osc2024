.section ".text"

.global _start
_start:
    mov     x8, 0 // get pid
    svc     0
    ret