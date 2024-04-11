/*.section ".text"
.global _start
_start:
	//讓在EL0的user program可以請求EL1的OS service
	svc	0x1337
1:
	nop
	b	1b*/

.section ".text"
.global _start
_start:
    mov x0, 0
1:
    add x0, x0, 1
    svc 0
    cmp x0, 5
    blt 1b
1:
    b 1b