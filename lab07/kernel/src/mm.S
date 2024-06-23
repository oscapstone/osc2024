.globl memzero_asm
memzero_asm:
    str xzr, [x0], #8
    subs x1, x1, #8
    b.gt memzero_asm
    ret