.section ".text"

.global enable_irq
enable_irq:
	msr    daifclr, 0xf
	ret

.global disable_irq
disable_irq:
	msr	daifset, 0xf
	ret