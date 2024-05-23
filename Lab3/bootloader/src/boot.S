.section ".text.boot"

.global _start

_start:

	mov     x10, x0
	mov     x11, x1
	mov     x12, x2
	mov     x13, x3

check_proc:

	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, not_hang		// Hang for all non-primary CPU
	wfe
	b 	proc_hang-0x20000

proc_hang:
	wfe
	b 	proc_hang-0x20000

not_hang:

clear_bss:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero
	
relocate:
	ldr x1, =init_addr
	ldr x2, =_bootloader_relocated_addr
	ldr w3, =__bootloader_size

	
relocate_loop:
	ldr x4, [x1], #8
	str x4, [x2], #8
	sub w3, w3, #1
	cbnz w3, relocate_loop

master:
	sev
	ldr x1, =_stack_top-0x20000
    	mov sp, x1
	bl	kernel_main-0x20000
	b 	proc_hang-0x20000	// should never come here

memzero:
	str xzr, [x0], #8
	subs x1, x1, #8
	b.gt memzero
	ret
