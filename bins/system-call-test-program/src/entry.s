.section .text._start
_start:
    b _start_rust

system_call:
    mov x8, x0
    mov x0, x1
    mov x1, x2
    mov x2, x3
    mov x3, x4
    mov x4, x5
    mov x5, x6
    svc 0
    ret

.type	_start, function
.type   system_call, function
.global	_start
.global system_call