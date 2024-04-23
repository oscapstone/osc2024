
.global utils_delay

utils_delay:
    subs    x0, x0, #1
    bne     utils_delay
    ret



