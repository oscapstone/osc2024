.globl delay
delay:
	subs x0, x0, #1 // Subtract 1 from x0 and update the flags based on the result
	bne delay		// If the result of the subtraction is not zero, branch back to the delay label
	ret