.global memzero
memzero:
	str xzr, [x0], #8
	subs x1, x1, #8
	bgt memzero
	ret
/* 
.global memcpy
memcpy:
	ldr x3, [x1], #8
	str x3, [x0], #8
	subs x2, x2, #8
	bgt memcpy
	ret*/