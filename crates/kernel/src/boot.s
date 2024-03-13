.section .text._start

_start:
.L_parking_loop:
    wfe
    b   .L_parking_loop

.size   _start, . - _start
.type   _start, function
.global _start
