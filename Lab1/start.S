.section ".text.boot"

.global _start

_start:
	//mpidr->cpu num
	mrs	x1, mpidr_el1
	//get the last 2 num
	and	x1, x1, #3
	//if cpu num > 0(!main cpu) go1 else go 2
	cbz	x1, 2f
1:	wfe			//wait for event
	b	1b		//jump backward -> halt

	//if is main cpu(id == 0)
	//set top of stack
2:	ldr	x1, =_start	//load _start to x1
	mov	sp, x1          //move x1(_start) to sp 
	//clear bss
	ldr	x1, =__bss_start
	ldr	w2, =__bss_size	//(byte)
3: 	cbz	w2, 4f		//compare the branch on zero if 0 -> go 4
	str	xzr, [x1], #8	//set 0 to x1's address then +8(bit)  
	sub	w2, w2, #1	//w2 = w2 - 1
	cbnz	w2, 3b		//compare the branch on non-zero if !0 -> go 3

4:	bl	main		//branch with link
	
	b	1b		//halt for safety
