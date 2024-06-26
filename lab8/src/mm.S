.globl memzero
memzero:
	str xzr, [x0], #8
	subs x1, x1, #8
	b.gt memzero
	ret

.globl memncpy
memncpy:
    b memncpy_1_cmp
memncpy_1:
    ldrb w3, [x1], #1
    strb w3, [x0], #1
    subs x2, x2, #1
memncpy_1_cmp:
    cmp x2, 1
    bge memncpy_1
memncpy_ok:
    ret