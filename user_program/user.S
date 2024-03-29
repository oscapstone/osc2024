.section ".text"
.global _start
_start:
    mov x6, 0
1:
    add x6, x6, 1
    svc 4
    cmp x6, 5
    blt 1b
2:
    b 2b