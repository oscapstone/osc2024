.section ".text"
.global _start
_start:
    mov x0, 0
1:
    add x0, x0, 1
    svc 0 // Software Interrupt
    cmp x0, 5 // 更新 PSR (program state reg)
    blt 1b // 根據 PSR 跳轉: N, V
1:
    b 1b