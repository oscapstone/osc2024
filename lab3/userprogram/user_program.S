.section ".text"
.global _start
_start:
    mov x0, 0
1:
    add x0, x0, 1
    svc 0           // system call el0 -> el1, record ESR_EL1 as 110_1010_0000,imm16
    cmp x0, 5
    blt 1b
1:
    b 1b