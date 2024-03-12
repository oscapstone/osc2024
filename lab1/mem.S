.global mem_zero
mem_zero:
// zero out memory x0(low), x1(size to zero out)
    str xzr, [x0], #8   // xzr is zero register
    subs x1, x1, #8          // compare if equal
    b.gt mem_zero
    ret
