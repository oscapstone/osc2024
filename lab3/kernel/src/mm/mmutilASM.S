

.global memzero

memzero:
    str     xzr, [x0], #8
    subs    x1, x1, #8
    b.gt    memzero
    ret

// memcpy(void* src, void* dst, size_t size);
.global memcpy
memcpy:
    ldr     x3, [x0], #8
    str     x3, [x1], #8
    sub     x2, x2, #8
    cbnz    x2, memcpy
    ret