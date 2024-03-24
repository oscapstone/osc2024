.globl mem_align
mem_align:
	sub x1, x1, #1
	add x0, x0, x1
	mvn x1, x1
	and x0, x0, x1
	ret
