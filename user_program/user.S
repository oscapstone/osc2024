.section ".text"
.global _start
_start:
    mov x5, 0
1:
    add x5, x5, 1
    svc 0
    cmp x5, 5
    blt 1b
2:
    b 2b