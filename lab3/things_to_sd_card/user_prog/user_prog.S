.section ".text"
.global _start
_start:
    mov x0, 0
1:
    add x0, x0, 1   // x0 = x0 + 1
    svc 0           // syscall(0) -> software interrupt
    cmp x0, 5       // if x0 < 5
    blt 1b          // then goto 1
1:                  // infinite loop
    b 1b