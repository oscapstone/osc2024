.globl put32
put32:
	str w1,[x0]		// write 32 bits data w1 to x0 address
	ret

.globl get32
get32:
	ldr w0,[x0]		// write 32 bits data and store to w0 from x0 address
	ret

.globl delay
delay:
	subs x0, x0, #1	// x0 = x0 - 1
	bne delay		// if x0 != 0, loop back to delay
	ret