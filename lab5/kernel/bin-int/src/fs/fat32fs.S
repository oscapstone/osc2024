	.arch armv8-a
	.file	"fat32fs.c"
	.text
	.global	fat32fs_f_ops
	.section	.data.rel.local,"aw"
	.align	3
	.type	fat32fs_f_ops, %object
	.size	fat32fs_f_ops, 48
fat32fs_f_ops:
	.xword	write
	.xword	read
	.xword	open
	.xword	close
	.xword	lseek64
	.xword	ioctl
	.global	fat32fs_v_ops
	.align	3
	.type	fat32fs_v_ops, %object
	.size	fat32fs_v_ops, 24
fat32fs_v_ops:
	.xword	lookup
	.xword	create
	.xword	mkdir
	.section	.rodata
	.align	3
.LC0:
	.string	"fat32"
	.align	3
.LC1:
	.string	"[FS][ERROR] FAT32fs: Does not have R/W function implement, cannot mount.\n"
	.align	3
.LC2:
	.string	"[SDCARD] Reading MBR info...\n"
	.align	3
.LC3:
	.string	"[SDCARD][ERROR] Failed to load MBR structure.\n"
	.align	3
.LC4:
	.string	"Partition 0 info addr: %p\n"
	.align	3
.LC5:
	.string	"Partition 0 boot sector index addr: %p\n"
	.align	3
.LC6:
	.string	"[FS][TRACE] Partition 0 boot sector index: %d\n"
	.align	3
.LC7:
	.string	"FAT32   "
	.align	3
.LC8:
	.string	"[FS][ERROR] FAT32fs: fat label is not correct.\n"
	.align	3
.LC9:
	.string	"[FS][TRACE] FAT table start sector: %d\n"
	.align	3
.LC10:
	.string	"[FS][TRACE] Data start sector: %d\n"
	.align	3
.LC11:
	.string	"[FS] FAT32fs: memory FAT table addr: %p\n"
	.align	3
.LC12:
	.string	"[FS] FAT32fs: mounting on %s\n"
	.align	3
.LC13:
	.string	"Root start cluster id: %d\n"
	.align	3
.LC14:
	.string	"[FS][ERROR] FAT32fs: Failed to initialize the root node.\n"
	.text
	.align	2
	.type	setup_mount, %function
setup_mount:
.LFB0:
	.cfi_startproc
	sub	sp, sp, #608
	.cfi_def_cfa_offset 608
	stp	x29, x30, [sp]
	.cfi_offset 29, -608
	.cfi_offset 30, -600
	mov	x29, sp
	str	x19, [sp, 16]
	.cfi_offset 19, -592
	str	x0, [sp, 40]
	str	x1, [sp, 32]
	ldr	x0, [sp, 40]
	ldr	x19, [x0]
	adrp	x0, .LC0
	add	x0, x0, :lo12:.LC0
	bl	utils_strlen
	mov	w2, w0
	adrp	x0, .LC0
	add	x1, x0, :lo12:.LC0
	mov	x0, x19
	bl	utils_strncmp
	cmp	w0, 0
	beq	.L2
	mov	w0, -1
	b	.L10
.L2:
	ldr	x0, [sp, 32]
	ldr	x0, [x0, 16]
	cmp	x0, 0
	cset	w0, eq
	and	w1, w0, 255
	ldr	x0, [sp, 32]
	ldr	x0, [x0, 24]
	cmp	x0, 0
	cset	w0, eq
	and	w0, w0, 255
	and	w0, w1, w0
	and	w0, w0, 255
	cmp	w0, 0
	beq	.L4
	adrp	x0, .LC1
	add	x0, x0, :lo12:.LC1
	bl	printf_
	mov	w0, -1
	b	.L10
.L4:
	adrp	x0, .LC2
	add	x0, x0, :lo12:.LC2
	bl	printf_
	ldr	x0, [sp, 32]
	ldr	x3, [x0, 16]
	add	x0, sp, 56
	mov	x2, 512
	mov	x1, x0
	mov	x0, 0
	blr	x3
	ldrb	w0, [sp, 566]
	cmp	w0, 85
	bne	.L5
	ldrb	w0, [sp, 567]
	cmp	w0, 170
	beq	.L6
.L5:
	adrp	x0, .LC3
	add	x0, x0, :lo12:.LC3
	bl	printf_
	mov	w0, -1
	b	.L10
.L6:
	ldr	x0, [sp, 32]
	ldr	x0, [x0, 32]
	cmp	x0, 0
	beq	.L7
	ldr	x0, [sp, 32]
	ldr	x0, [x0, 32]
	bl	kfree
.L7:
	mov	x0, 112
	bl	kzalloc
	mov	x1, x0
	ldr	x0, [sp, 32]
	str	x1, [x0, 32]
	ldr	x0, [sp, 32]
	ldr	x0, [x0, 32]
	str	x0, [sp, 600]
	add	x0, sp, 56
	add	x0, x0, 446
	str	x0, [sp, 592]
	ldr	x1, [sp, 592]
	adrp	x0, .LC4
	add	x0, x0, :lo12:.LC4
	bl	printf_
	ldr	x0, [sp, 592]
	add	x0, x0, 8
	str	x0, [sp, 584]
	ldr	x1, [sp, 592]
	adrp	x0, .LC5
	add	x0, x0, :lo12:.LC5
	bl	printf_
	ldr	x0, [sp, 584]
	ldr	w0, [x0]
	str	w0, [sp, 580]
	ldr	w1, [sp, 580]
	adrp	x0, .LC6
	add	x0, x0, :lo12:.LC6
	bl	printf_
	ldr	x0, [sp, 32]
	ldr	x3, [x0, 16]
	ldr	w0, [sp, 580]
	lsl	w0, w0, 9
	uxtw	x0, w0
	ldr	x1, [sp, 600]
	mov	x2, 90
	blr	x3
	ldr	x0, [sp, 600]
	add	x3, x0, 82
	mov	w2, 8
	adrp	x0, .LC7
	add	x1, x0, :lo12:.LC7
	mov	x0, x3
	bl	utils_strncmp
	cmp	w0, 0
	beq	.L8
	adrp	x0, .LC8
	add	x0, x0, :lo12:.LC8
	bl	printf_
	mov	w0, -1
	b	.L10
.L8:
	ldr	x0, [sp, 600]
	ldrh	w0, [x0, 14]
	mov	w1, w0
	ldr	w0, [sp, 580]
	add	w1, w1, w0
	ldr	x0, [sp, 600]
	str	w1, [x0, 92]
	ldr	x0, [sp, 600]
	ldr	w1, [x0, 92]
	ldr	x0, [sp, 600]
	ldrb	w0, [x0, 16]
	mov	w2, w0
	ldr	x0, [sp, 600]
	ldr	w0, [x0, 36]
	mul	w0, w2, w0
	add	w0, w1, w0
	sub	w1, w0, #2
	ldr	x0, [sp, 600]
	str	w1, [x0, 96]
	ldr	x0, [sp, 600]
	ldr	w0, [x0, 92]
	mov	w1, w0
	adrp	x0, .LC9
	add	x0, x0, :lo12:.LC9
	bl	printf_
	ldr	x0, [sp, 600]
	ldr	w0, [x0, 96]
	mov	w1, w0
	adrp	x0, .LC10
	add	x0, x0, :lo12:.LC10
	bl	printf_
	ldr	x0, [sp, 600]
	ldrb	w0, [x0, 16]
	mov	w1, w0
	ldr	x0, [sp, 600]
	ldr	w0, [x0, 36]
	mul	w0, w1, w0
	str	w0, [sp, 576]
	ldr	x0, [sp, 600]
	ldrh	w0, [x0, 11]
	mov	w1, w0
	ldr	w0, [sp, 576]
	mul	w0, w1, w0
	uxtw	x0, w0
	bl	kzalloc
	mov	x1, x0
	ldr	x0, [sp, 600]
	str	x1, [x0, 104]
	ldr	x0, [sp, 32]
	ldr	x3, [x0, 16]
	ldr	x0, [sp, 600]
	ldr	w0, [x0, 92]
	lsl	w0, w0, 9
	uxtw	x4, w0
	ldr	x0, [sp, 600]
	ldr	x1, [x0, 104]
	ldr	x0, [sp, 600]
	ldrh	w0, [x0, 11]
	mov	w2, w0
	ldr	w0, [sp, 576]
	mul	w0, w2, w0
	uxtw	x0, w0
	mov	x2, x0
	mov	x0, x4
	blr	x3
	ldr	x0, [sp, 600]
	ldr	x0, [x0, 104]
	mov	x1, x0
	adrp	x0, .LC11
	add	x0, x0, :lo12:.LC11
	bl	printf_
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	ldr	x0, [x0, 72]
	mov	x1, x0
	adrp	x0, .LC12
	add	x0, x0, :lo12:.LC12
	bl	printf_
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	ldr	x1, [sp, 32]
	str	x1, [x0]
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	adrp	x1, fat32fs_f_ops
	add	x1, x1, :lo12:fat32fs_f_ops
	str	x1, [x0, 16]
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	adrp	x1, fat32fs_v_ops
	add	x1, x1, :lo12:fat32fs_v_ops
	str	x1, [x0, 8]
	ldr	x0, [sp, 32]
	ldr	x1, [sp, 40]
	str	x1, [x0, 8]
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	add	x0, x0, 24
	bl	link_list_init
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	str	xzr, [x0, 40]
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	str	xzr, [x0, 88]
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	str	xzr, [x0, 96]
	mov	x0, 12
	bl	kzalloc
	str	x0, [sp, 568]
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	ldr	x1, [sp, 568]
	str	x1, [x0, 104]
	ldr	x0, [sp, 600]
	ldr	w1, [x0, 44]
	ldr	x0, [sp, 568]
	str	w1, [x0]
	ldr	x0, [sp, 568]
	ldr	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC13
	add	x0, x0, :lo12:.LC13
	bl	printf_
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	bl	init_vnode
	cmn	w0, #1
	bne	.L9
	adrp	x0, .LC14
	add	x0, x0, :lo12:.LC14
	bl	printf_
	mov	w0, -1
	b	.L10
.L9:
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	bl	parse_dir_entries
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	ldr	w1, [x0, 80]
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	and	w1, w1, -32769
	str	w1, [x0, 80]
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	ldr	w1, [x0, 80]
	ldr	x0, [sp, 32]
	ldr	x0, [x0]
	orr	w1, w1, 16384
	str	w1, [x0, 80]
	mov	w0, 0
.L10:
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp]
	add	sp, sp, 608
	.cfi_restore 29
	.cfi_restore 30
	.cfi_restore 19
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE0:
	.size	setup_mount, .-setup_mount
	.section	.rodata
	.align	3
.LC15:
	.string	"Bad cluster.\n"
	.text
	.align	2
	.type	get_chain_len, %function
get_chain_len:
.LFB1:
	.cfi_startproc
	stp	x29, x30, [sp, -64]!
	.cfi_def_cfa_offset 64
	.cfi_offset 29, -64
	.cfi_offset 30, -56
	mov	x29, sp
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0]
	str	x0, [sp, 48]
	ldr	x0, [sp, 48]
	ldr	x0, [x0, 32]
	str	x0, [sp, 40]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 104]
	ldr	w0, [x0]
	str	w0, [sp, 60]
	mov	w0, 1
	str	w0, [sp, 56]
.L15:
	ldr	x0, [sp, 40]
	ldr	x1, [x0, 104]
	ldr	w0, [sp, 60]
	lsl	x0, x0, 2
	add	x0, x1, x0
	ldr	w0, [x0]
	str	w0, [sp, 60]
	ldr	w1, [sp, 60]
	mov	w0, 268435448
	cmp	w1, w0
	bcc	.L12
	ldr	w1, [sp, 60]
	mov	w0, 268435455
	cmp	w1, w0
	bhi	.L12
	ldr	w0, [sp, 56]
	b	.L13
.L12:
	ldr	w1, [sp, 60]
	mov	w0, 65527
	movk	w0, 0xfff, lsl 16
	cmp	w1, w0
	bne	.L14
	adrp	x0, .LC15
	add	x0, x0, :lo12:.LC15
	bl	printf_
	ldr	w0, [sp, 56]
	b	.L13
.L14:
	ldr	w0, [sp, 56]
	add	w0, w0, 1
	str	w0, [sp, 56]
	b	.L15
.L13:
	ldp	x29, x30, [sp], 64
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE1:
	.size	get_chain_len, .-get_chain_len
	.section	.rodata
	.align	3
.LC16:
	.string	"[FS] FAT32fs: init_vnode(): start index: %d\n"
	.align	3
.LC17:
	.string	"[FS] FAT32fs: init_vnode(): next index: %d\n"
	.text
	.align	2
	.type	init_vnode, %function
init_vnode:
.LFB2:
	.cfi_startproc
	stp	x29, x30, [sp, -80]!
	.cfi_def_cfa_offset 80
	.cfi_offset 29, -80
	.cfi_offset 30, -72
	mov	x29, sp
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0]
	ldr	x0, [x0, 32]
	str	x0, [sp, 64]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 104]
	str	x0, [sp, 56]
	ldr	x0, [sp, 24]
	bl	get_chain_len
	str	w0, [sp, 52]
	ldr	x0, [sp, 64]
	ldrh	w0, [x0, 11]
	mov	w1, w0
	ldr	w0, [sp, 52]
	mul	w0, w0, w1
	str	w0, [sp, 48]
	ldr	w0, [sp, 48]
	bl	kzalloc
	str	x0, [sp, 40]
	ldr	x0, [sp, 56]
	ldr	w0, [x0]
	str	w0, [sp, 76]
	ldr	w1, [sp, 76]
	adrp	x0, .LC16
	add	x0, x0, :lo12:.LC16
	bl	printf_
	str	wzr, [sp, 72]
	b	.L17
.L18:
	ldr	x0, [sp, 24]
	ldr	x0, [x0]
	ldr	x3, [x0, 16]
	ldr	x0, [sp, 64]
	ldr	w1, [x0, 96]
	ldr	w0, [sp, 76]
	add	w0, w1, w0
	ldr	x1, [sp, 64]
	ldrh	w1, [x1, 11]
	mul	w0, w0, w1
	uxtw	x4, w0
	ldr	x0, [sp, 64]
	ldrh	w0, [x0, 11]
	mov	w1, w0
	ldr	w0, [sp, 72]
	mul	w0, w1, w0
	uxtw	x0, w0
	ldr	x1, [sp, 40]
	add	x1, x1, x0
	ldr	x0, [sp, 64]
	ldrh	w0, [x0, 11]
	and	x0, x0, 65535
	mov	x2, x0
	mov	x0, x4
	blr	x3
	ldr	x0, [sp, 64]
	ldr	x1, [x0, 104]
	ldr	w0, [sp, 76]
	lsl	x0, x0, 2
	add	x0, x1, x0
	ldr	w0, [x0]
	str	w0, [sp, 76]
	ldr	w1, [sp, 76]
	adrp	x0, .LC17
	add	x0, x0, :lo12:.LC17
	bl	printf_
	ldr	w0, [sp, 72]
	add	w0, w0, 1
	str	w0, [sp, 72]
.L17:
	ldr	w1, [sp, 72]
	ldr	w0, [sp, 52]
	cmp	w1, w0
	bcc	.L18
	ldr	x0, [sp, 56]
	ldr	w1, [sp, 52]
	str	w1, [x0, 4]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 88]
	cmp	x0, 0
	beq	.L19
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 88]
	bl	kfree
.L19:
	ldr	x0, [sp, 24]
	ldr	x1, [sp, 40]
	str	x1, [x0, 88]
	ldr	w1, [sp, 48]
	ldr	x0, [sp, 24]
	str	x1, [x0, 96]
	ldr	x0, [sp, 56]
	strb	wzr, [x0, 8]
	ldr	x0, [sp, 56]
	mov	w1, 1
	strb	w1, [x0, 9]
	mov	w0, 0
	ldp	x29, x30, [sp], 80
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE2:
	.size	init_vnode, .-init_vnode
	.align	2
	.global	fat32fs_create
	.type	fat32fs_create, %function
fat32fs_create:
.LFB3:
	.cfi_startproc
	stp	x29, x30, [sp, -32]!
	.cfi_def_cfa_offset 32
	.cfi_offset 29, -32
	.cfi_offset 30, -24
	mov	x29, sp
	mov	x0, 32
	bl	kmalloc
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	adrp	x1, .LC0
	add	x1, x1, :lo12:.LC0
	str	x1, [x0]
	ldr	x0, [sp, 24]
	adrp	x1, setup_mount
	add	x1, x1, :lo12:setup_mount
	str	x1, [x0, 8]
	ldr	x0, [sp, 24]
	add	x0, x0, 16
	bl	link_list_init
	ldr	x0, [sp, 24]
	ldp	x29, x30, [sp], 32
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE3:
	.size	fat32fs_create, .-fat32fs_create
	.align	2
	.type	open, %function
open:
.LFB4:
	.cfi_startproc
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	str	x0, [sp, 8]
	str	x1, [sp]
	mov	w0, 0
	add	sp, sp, 16
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE4:
	.size	open, .-open
	.align	2
	.type	close, %function
close:
.LFB5:
	.cfi_startproc
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	str	x0, [sp, 8]
	mov	w0, 0
	add	sp, sp, 16
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE5:
	.size	close, .-close
	.align	2
	.type	read, %function
read:
.LFB6:
	.cfi_startproc
	sub	sp, sp, #48
	.cfi_def_cfa_offset 48
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	str	x2, [sp, 8]
	ldr	x0, [sp, 24]
	ldr	x0, [x0]
	str	x0, [sp, 40]
	ldr	x0, [sp, 40]
	ldr	w0, [x0, 80]
	and	w0, w0, 61440
	cmp	w0, 32768
	beq	.L28
	mov	w0, -1
	b	.L29
.L28:
	ldr	x0, [sp, 40]
	ldr	x1, [x0, 96]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 8]
	sub	x0, x1, x0
	ldr	x2, [sp, 8]
	ldr	x1, [sp, 8]
	cmp	x2, x0
	csel	x0, x1, x0, ls
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	cmp	x0, 0
	bne	.L30
	mov	w0, -1
	b	.L29
.L30:
	mov	w0, -1
.L29:
	add	sp, sp, 48
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE6:
	.size	read, .-read
	.align	2
	.type	write, %function
write:
.LFB7:
	.cfi_startproc
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	str	x2, [sp, 8]
	mov	w0, -1
	add	sp, sp, 32
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE7:
	.size	write, .-write
	.align	2
	.type	lseek64, %function
lseek64:
.LFB8:
	.cfi_startproc
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	str	w2, [sp, 12]
	ldr	w0, [sp, 12]
	cmp	w0, 0
	bne	.L34
	ldr	x1, [sp, 16]
	ldr	x0, [sp, 24]
	str	x1, [x0, 8]
	ldr	x0, [sp, 16]
	b	.L35
.L34:
	mov	x0, -1
.L35:
	add	sp, sp, 32
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE8:
	.size	lseek64, .-lseek64
	.section	.rodata
	.align	3
.LC18:
	.string	"[FS][ERROR] FAT32fs: lookup(): directory node is not a directory. name: %s\n"
	.text
	.align	2
	.type	lookup, %function
lookup:
.LFB9:
	.cfi_startproc
	stp	x29, x30, [sp, -96]!
	.cfi_def_cfa_offset 96
	.cfi_offset 29, -96
	.cfi_offset 30, -88
	mov	x29, sp
	str	x19, [sp, 16]
	.cfi_offset 19, -80
	str	x0, [sp, 56]
	str	x1, [sp, 48]
	str	x2, [sp, 40]
	ldr	x0, [sp, 56]
	cmp	x0, 0
	bne	.L37
	mov	w0, -1
	b	.L38
.L37:
	ldr	x0, [sp, 56]
	ldr	w0, [x0, 80]
	and	w0, w0, 61440
	cmp	w0, 16384
	beq	.L39
	ldr	x0, [sp, 56]
	ldr	x0, [x0, 72]
	mov	x1, x0
	adrp	x0, .LC18
	add	x0, x0, :lo12:.LC18
	bl	printf_
	mov	w0, -1
	b	.L38
.L39:
	ldr	x0, [sp, 56]
	ldr	x0, [x0, 40]
	cmp	x0, 0
	bne	.L40
	ldr	x0, [sp, 56]
	bl	parse_dir_entries
.L40:
	ldr	x0, [sp, 56]
	ldr	x0, [x0, 104]
	str	x0, [sp, 80]
	str	xzr, [sp, 88]
	ldr	x0, [sp, 56]
	ldr	x0, [x0, 32]
	str	x0, [sp, 72]
	ldr	x0, [sp, 72]
	sub	x0, x0, #48
	str	x0, [sp, 88]
	b	.L41
.L44:
	ldr	x0, [sp, 88]
	ldr	x19, [x0, 72]
	ldr	x0, [sp, 88]
	ldr	x0, [x0, 72]
	bl	utils_strlen
	mov	w2, w0
	ldr	x1, [sp, 40]
	mov	x0, x19
	bl	utils_strncmp
	cmp	w0, 0
	bne	.L42
	ldr	x0, [sp, 88]
	ldr	x0, [x0, 88]
	cmp	x0, 0
	bne	.L43
	ldr	x0, [sp, 88]
	bl	init_vnode
.L43:
	ldr	x0, [sp, 48]
	ldr	x1, [sp, 88]
	str	x1, [x0]
	mov	w0, 0
	b	.L38
.L42:
	ldr	x0, [sp, 88]
	ldr	x0, [x0, 56]
	str	x0, [sp, 64]
	ldr	x0, [sp, 64]
	sub	x0, x0, #48
	str	x0, [sp, 88]
.L41:
	ldr	x0, [sp, 88]
	add	x1, x0, 48
	ldr	x0, [sp, 56]
	add	x0, x0, 24
	cmp	x1, x0
	bne	.L44
	mov	w0, -1
.L38:
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 96
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 19
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE9:
	.size	lookup, .-lookup
	.align	2
	.type	create, %function
create:
.LFB10:
	.cfi_startproc
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	str	x2, [sp, 8]
	mov	w0, -1
	add	sp, sp, 32
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE10:
	.size	create, .-create
	.align	2
	.type	mkdir, %function
mkdir:
.LFB11:
	.cfi_startproc
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	str	x2, [sp, 8]
	mov	w0, -1
	add	sp, sp, 32
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE11:
	.size	mkdir, .-mkdir
	.align	2
	.type	ioctl, %function
ioctl:
.LFB12:
	.cfi_startproc
	sub	sp, sp, #64
	.cfi_def_cfa_offset 64
	str	x0, [sp, 8]
	str	x1, [sp]
	str	x2, [sp, 16]
	str	x3, [sp, 24]
	str	x4, [sp, 32]
	str	x5, [sp, 40]
	str	x6, [sp, 48]
	str	x7, [sp, 56]
	mov	w0, -1
	add	sp, sp, 64
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE12:
	.size	ioctl, .-ioctl
	.section	.rodata
	.align	3
.LC19:
	.string	"ori file name:%s\n"
	.align	3
.LC20:
	.string	"ori ext name: %s\n"
	.align	3
.LC21:
	.string	"convert name: %s\n"
	.text
	.align	2
	.type	convert_fat_name, %function
convert_fat_name:
.LFB13:
	.cfi_startproc
	stp	x29, x30, [sp, -64]!
	.cfi_def_cfa_offset 64
	.cfi_offset 29, -64
	.cfi_offset 30, -56
	mov	x29, sp
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	str	xzr, [sp, 56]
	ldr	x0, [sp, 24]
	str	x0, [sp, 40]
	str	wzr, [sp, 52]
	b	.L52
.L54:
	ldrsw	x0, [sp, 52]
	ldr	x1, [sp, 40]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 32
	beq	.L53
	ldrsw	x0, [sp, 52]
	ldr	x1, [sp, 40]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 0
	beq	.L53
	ldrsw	x0, [sp, 52]
	ldr	x1, [sp, 40]
	add	x1, x1, x0
	ldr	x0, [sp, 56]
	add	x2, x0, 1
	str	x2, [sp, 56]
	ldr	x2, [sp, 16]
	add	x0, x2, x0
	ldrb	w1, [x1]
	strb	w1, [x0]
	ldr	w0, [sp, 52]
	add	w0, w0, 1
	str	w0, [sp, 52]
.L52:
	ldr	w0, [sp, 52]
	cmp	w0, 7
	ble	.L54
.L53:
	ldr	x0, [sp, 56]
	cmp	x0, 0
	bne	.L55
	mov	w0, -1
	b	.L56
.L55:
	ldr	x0, [sp, 56]
	add	x1, x0, 1
	str	x1, [sp, 56]
	ldr	x1, [sp, 16]
	add	x0, x1, x0
	mov	w1, 46
	strb	w1, [x0]
	ldr	x0, [sp, 24]
	add	x0, x0, 8
	str	x0, [sp, 32]
	str	wzr, [sp, 48]
	b	.L57
.L60:
	ldrsw	x0, [sp, 48]
	ldr	x1, [sp, 32]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 32
	beq	.L61
	ldrsw	x0, [sp, 48]
	ldr	x1, [sp, 32]
	add	x1, x1, x0
	ldr	x0, [sp, 56]
	add	x2, x0, 1
	str	x2, [sp, 56]
	ldr	x2, [sp, 16]
	add	x0, x2, x0
	ldrb	w1, [x1]
	strb	w1, [x0]
	ldr	w0, [sp, 48]
	add	w0, w0, 1
	str	w0, [sp, 48]
.L57:
	ldr	w0, [sp, 48]
	cmp	w0, 2
	ble	.L60
	b	.L59
.L61:
	nop
.L59:
	ldr	x1, [sp, 16]
	ldr	x0, [sp, 56]
	add	x0, x1, x0
	strb	wzr, [x0]
	ldr	x0, [sp, 24]
	mov	x1, x0
	adrp	x0, .LC19
	add	x0, x0, :lo12:.LC19
	bl	printf_
	ldr	x0, [sp, 24]
	add	x0, x0, 8
	mov	x1, x0
	adrp	x0, .LC20
	add	x0, x0, :lo12:.LC20
	bl	printf_
	ldr	x1, [sp, 16]
	adrp	x0, .LC21
	add	x0, x0, :lo12:.LC21
	bl	printf_
	mov	w0, 0
.L56:
	ldp	x29, x30, [sp], 64
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE13:
	.size	convert_fat_name, .-convert_fat_name
	.section	.rodata
	.align	3
.LC22:
	.string	"[FS][ERROR] FAT32fs: File content is not loaded. can not parse dir entries.\n"
	.text
	.align	2
	.type	parse_dir_entries, %function
parse_dir_entries:
.LFB14:
	.cfi_startproc
	stp	x29, x30, [sp, -96]!
	.cfi_def_cfa_offset 96
	.cfi_offset 29, -96
	.cfi_offset 30, -88
	mov	x29, sp
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 88]
	cmp	x0, 0
	bne	.L63
	adrp	x0, .LC22
	add	x0, x0, :lo12:.LC22
	bl	printf_
	mov	w0, -1
	b	.L64
.L63:
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 104]
	str	x0, [sp, 80]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 88]
	str	x0, [sp, 88]
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 88]
	mov	x1, x0
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 96]
	add	x0, x1, x0
	str	x0, [sp, 72]
	b	.L65
.L70:
	add	x0, sp, 40
	mov	x1, x0
	ldr	x0, [sp, 88]
	bl	convert_fat_name
	cmn	w0, #1
	beq	.L71
	add	x0, sp, 40
	mov	w1, 0
	bl	vnode_create
	str	x0, [sp, 64]
	ldr	x0, [sp, 88]
	ldrb	w0, [x0, 11]
	and	w0, w0, 16
	cmp	w0, 0
	beq	.L68
	ldr	x0, [sp, 64]
	mov	w1, 16384
	str	w1, [x0, 80]
	b	.L69
.L68:
	ldr	x0, [sp, 64]
	mov	w1, 32768
	str	w1, [x0, 80]
.L69:
	mov	x0, 12
	bl	kzalloc
	mov	x1, x0
	ldr	x0, [sp, 64]
	str	x1, [x0, 104]
	ldr	x0, [sp, 64]
	ldr	x0, [x0, 104]
	str	x0, [sp, 56]
	ldr	x0, [sp, 88]
	ldrh	w0, [x0, 20]
	lsl	w0, w0, 16
	ldr	x1, [sp, 88]
	ldrh	w1, [x1, 26]
	add	w0, w0, w1
	mov	w1, w0
	ldr	x0, [sp, 56]
	str	w1, [x0]
	ldr	x0, [sp, 56]
	strb	wzr, [x0, 8]
	ldr	x0, [sp, 56]
	strb	wzr, [x0, 9]
	ldr	x0, [sp, 88]
	ldr	w0, [x0, 28]
	uxtw	x1, w0
	ldr	x0, [sp, 64]
	str	x1, [x0, 96]
	ldr	x0, [sp, 24]
	ldr	x1, [x0]
	ldr	x0, [sp, 64]
	str	x1, [x0]
	ldr	x0, [sp, 24]
	ldr	x1, [x0, 8]
	ldr	x0, [sp, 64]
	str	x1, [x0, 8]
	ldr	x0, [sp, 24]
	ldr	x1, [x0, 16]
	ldr	x0, [sp, 64]
	str	x1, [x0, 16]
	ldr	x0, [sp, 64]
	ldr	x1, [sp, 24]
	str	x1, [x0, 64]
	ldr	x0, [sp, 24]
	add	x2, x0, 24
	ldr	x0, [sp, 64]
	add	x0, x0, 48
	mov	x1, x0
	mov	x0, x2
	bl	link_list_push_back
	ldr	x0, [sp, 24]
	ldr	x0, [x0, 40]
	add	x1, x0, 1
	ldr	x0, [sp, 24]
	str	x1, [x0, 40]
	ldr	x0, [sp, 88]
	add	x0, x0, 32
	str	x0, [sp, 88]
.L65:
	ldr	x0, [sp, 88]
	ldr	x1, [sp, 72]
	cmp	x1, x0
	bhi	.L70
	b	.L67
.L71:
	nop
.L67:
	mov	w0, 0
.L64:
	ldp	x29, x30, [sp], 96
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE14:
	.size	parse_dir_entries, .-parse_dir_entries
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
