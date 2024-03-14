.globl memzero
memzero:
	str xzr, [x0], #8 // Store 8 bytes of zeros at the address in x0, then increment x0 by 8
	subs x1, x1, #8 // Subtract 8 from the size (x1), setting flags based on the result
	b.gt memzero // If there are still bytes left to zero (x1 > 0), loop back to the start of memzero
	ret