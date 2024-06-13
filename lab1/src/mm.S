.globl memzero
memzero:
	str xzr, [x0], #8 // store reg imm, xzr: 64 bit zero-reg, store xzr to [x0], x0 = x0 + 8
	subs x1, x1, #8
	b.gt memzero
	ret
