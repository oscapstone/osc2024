.section ".text.boot"

.global _start
_prologue:
	mov x20, x0
	mov x1, #0x0000
	movk x1, #5, lsl #16	// 0x50000
	str x20, [x1]

_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF	// Check processor id
	cbz	x0, master		// if cpu_id == 0, then booting
	b	proc_hang       // Otherwise, hang

proc_hang: 
	b 	proc_hang

master:
	adr	x0, __bss_start
	adr	x1, __bss_size
	bl 	memzero

	adr x1, _start
	mov sp, x1
	bl	main     // jump to kernel_main c code
	b 	proc_hang		// should never come here

memzero:
    str xzr, [x0], #8   // xzr: a register holds zero
	subs x1, x1, #8
	b.gt memzero
	ret

// adr: Load a label's relative address into the target register.