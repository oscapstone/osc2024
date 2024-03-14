.globl put32
put32:
	str w1,[x0] // Store the 32-bit value in w1 to the memory address specified in x0
	ret

.globl get32
get32:
	ldr w0,[x0] // Load a 32-bit value from the memory address specified in x0 into w0
	ret

.globl delay
delay:
	subs x0, x0, #1 // Subtract 1 from x0 and update the flags based on the result
	bne delay		// If the result of the subtraction is not zero, branch back to the delay label
	ret