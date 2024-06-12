
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
