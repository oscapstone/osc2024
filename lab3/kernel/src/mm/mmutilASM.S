

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

// memset(void* src, U8 c, size_t size);
.global memset
memset:
    cmp     x2, #0
    beq     end_memset
memset_loop:
    strb    w1, [x0], #1
    subs    x2, x2, #1
    bne     memset_loop
end_memset:
    ret

