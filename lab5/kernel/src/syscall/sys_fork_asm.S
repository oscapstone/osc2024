	.arch armv8-a
	.file	"sys_fork.c"
	.text
	.section	.rodata
	.align	3
.LC0:
	.string	"[SYSCALL][TRACE] fork called pid = %d\n"
	.align	3
.LC1:
	.string	"%s_child"
	.align	3
.LC2:
	.string	"[FORK] split pid: %d\n"
	.align	3
.LC3:
	.string	"child process fork!\n"
	.align	3
.LC4:
	.string	"child SP: 0x%x, FP: 0x%x\n"
	.align	3
.LC5:
	.string	"new task GPD addr: 0x%x\n"
	.align	3
.LC6:
	.string	"[FORK] split pid: %d, parent_pid: %d, new_task_pid: %d\n"
	.align	3
.LC7:
	.string	"new_task SP: 0x%lx, FP: 0x%lx\n"
	.text
	.align	2
	.global	sys_fork
	.type	sys_fork, %function
sys_fork:
.LFB0:
	.cfi_startproc
	stp	x29, x30, [sp, -96]!
	.cfi_def_cfa_offset 96
	.cfi_offset 29, -96
	.cfi_offset 30, -88
	mov	x29, sp
	str	x0, [sp, 24]
	bl	task_get_current_el1
	str	x0, [sp, 88]
	ldr	x0, [sp, 88]
	ldr	x0, [x0, 112]
	mov	x1, x0
	adrp	x0, .LC0
	add	x0, x0, :lo12:.LC0
	bl	tfp_printf
	ldr	x0, [sp, 88]
	ldr	x0, [x0, 112]
	str	w0, [sp, 84]
	ldr	x0, [sp, 88]
	add	x0, x0, 152
	add	x3, sp, 40
	mov	x2, x0
	adrp	x0, .LC1
	add	x1, x0, :lo12:.LC1
	mov	x0, x3
	bl	tfp_sprintf
	bl	lock_interrupt
	ldr	x0, [sp, 88]
	ldr	w1, [x0, 120]
	add	x0, sp, 40
	bl	task_create_user
	str	x0, [sp, 72]
	ldr	x0, [sp, 88]
	ldr	w1, [x0, 124]
	ldr	x0, [sp, 72]
	str	w1, [x0, 124]
	ldrsw	x1, [sp, 84]
	ldr	x0, [sp, 72]
	str	x1, [x0, 176]
	ldr	x1, [sp, 72]
	ldr	x0, [sp, 88]
	bl	mmu_fork_mm
	ldr	x0, [sp, 72]
	bl	task_run
	ldr	x0, [sp, 88]
	bl	task_asm_store_context
	bl	task_get_current_el1
	str	x0, [sp, 88]
	ldr	x0, [sp, 88]
	ldr	x0, [x0, 112]
	mov	x1, x0
	adrp	x0, .LC2
	add	x0, x0, :lo12:.LC2
	bl	tfp_printf
	ldrsw	x1, [sp, 84]
	ldr	x0, [sp, 88]
	ldr	x0, [x0, 112]
	cmp	x1, x0
	beq	.L2
	adrp	x0, .LC3
	add	x0, x0, :lo12:.LC3
	bl	tfp_printf
	ldr	x0, [sp, 88]
	ldr	x1, [x0, 96]
	ldr	x0, [sp, 88]
	ldr	x0, [x0, 80]
	mov	x2, x0
	adrp	x0, .LC4
	add	x0, x0, :lo12:.LC4
	bl	tfp_printf
	ldr	x0, [sp, 24]
	str	xzr, [x0, 32]
	b	.L1
.L2:
	ldr	x0, [sp, 88]
	ldr	x3, [x0, 136]
	ldr	x0, [sp, 72]
	ldr	x0, [x0, 136]
	mov	x2, 16384
	mov	x1, x0
	mov	x0, x3
	bl	memcpy
	ldr	x0, [sp, 88]
	ldr	x3, [x0, 144]
	ldr	x0, [sp, 72]
	ldr	x0, [x0, 144]
	mov	x2, 16384
	mov	x1, x0
	mov	x0, x3
	bl	memcpy
	ldr	x0, [sp, 72]
	ldr	x0, [x0, 104]
	str	x0, [sp, 64]
	ldr	x0, [sp, 88]
	ldr	x1, [sp, 72]
	mov	x2, 112
	bl	memcpy
	ldr	x1, [sp, 64]
	mov	x0, 281474976710656
	add	x1, x1, x0
	ldr	x0, [sp, 72]
	str	x1, [x0, 104]
	ldr	x0, [sp, 72]
	ldr	x0, [x0, 104]
	mov	x1, x0
	adrp	x0, .LC5
	add	x0, x0, :lo12:.LC5
	bl	tfp_printf
	ldr	x0, [sp, 72]
	ldr	x1, [x0, 80]
	ldr	x0, [sp, 72]
	ldr	x0, [x0, 136]
	mov	x2, x0
	ldr	x0, [sp, 88]
	ldr	x0, [x0, 136]
	sub	x0, x2, x0
	add	x1, x1, x0
	ldr	x0, [sp, 72]
	str	x1, [x0, 80]
	ldr	x0, [sp, 72]
	ldr	x1, [x0, 96]
	ldr	x0, [sp, 72]
	ldr	x0, [x0, 136]
	mov	x2, x0
	ldr	x0, [sp, 88]
	ldr	x0, [x0, 136]
	sub	x0, x2, x0
	add	x1, x1, x0
	ldr	x0, [sp, 72]
	str	x1, [x0, 96]
	ldr	x0, [sp, 88]
	ldr	x1, [x0, 112]
	ldr	x0, [sp, 72]
	ldr	x0, [x0, 112]
	mov	x3, x0
	ldr	w2, [sp, 84]
	adrp	x0, .LC6
	add	x0, x0, :lo12:.LC6
	bl	tfp_printf
	ldr	x0, [sp, 72]
	ldr	x1, [x0, 96]
	ldr	x0, [sp, 72]
	ldr	x0, [x0, 80]
	mov	x2, x0
	adrp	x0, .LC7
	add	x0, x0, :lo12:.LC7
	bl	tfp_printf
	bl	unlock_interrupt
	ldr	x0, [sp, 72]
	ldr	x1, [x0, 112]
	ldr	x0, [sp, 24]
	str	x1, [x0, 32]
	nop
.L1:
	ldp	x29, x30, [sp], 96
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE0:
	.size	sys_fork, .-sys_fork
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
