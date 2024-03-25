.globl get32
get32:
	ldr w0,[x0]
	ret

.globl set32
set32:
	str w1,[x0]
	ret

.globl wait_cycle
wait_cycle:
	subs x0, x0, #1
	bne wait_cycle
	ret
