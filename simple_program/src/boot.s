.section .text._start

.global _start
.global _bss_start
.global _bss_end

_start:
    # Initialize bss
    ldr x0, =_bss_start
    ldr x1, =_bss_end
.L_clear_bss:
    cmp x0, x1
    bge .L_done_clearing
    str xzr, [x0], #8
    b .L_clear_bss

.L_done_clearing:

    b main