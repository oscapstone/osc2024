
.global utils_delay

utils_delay:
    subs    x0, x0, #1
    bne     utils_delay
    ret


.global utils_get_el
utils_get_el:
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    ret
