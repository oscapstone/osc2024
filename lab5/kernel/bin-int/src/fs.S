	.arch armv8-a
	.file	"fs.c"
	.text
	.global	fs_manager
	.bss
	.align	3
	.type	fs_manager, %object
	.size	fs_manager, 8
fs_manager:
	.zero	8
	.section	.rodata
	.align	3
.LC0:
	.string	".."
	.align	3
.LC1:
	.string	"[FS] fs_find_node(): error no parent!\n"
	.align	3
.LC2:
	.string	"."
	.align	3
.LC3:
	.string	"/"
	.align	3
.LC4:
	.string	"[FS] fs_find_node(): file name not found. name: %s, path: %s\n"
	.align	3
.LC5:
	.string	"[FS] fs_find_node(): file is not a directory. name: %s, path: %s\n"
	.text
	.align	2
	.global	fs_find_node
	.type	fs_find_node, %function
fs_find_node:
.LFB0:
	.cfi_startproc
	stp	x29, x30, [sp, -368]!
	.cfi_def_cfa_offset 368
	.cfi_offset 29, -368
	.cfi_offset 30, -360
	mov	x29, sp
	str	x0, [sp, 56]
	str	x1, [sp, 48]
	str	x2, [sp, 40]
	str	x3, [sp, 32]
	str	x4, [sp, 24]
	str	xzr, [sp, 360]
	ldr	x0, [sp, 56]
	str	x0, [sp, 352]
	str	xzr, [sp, 344]
	ldr	x1, [sp, 48]
	ldr	x0, [sp, 344]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 47
	beq	.L2
	ldr	x0, [sp, 352]
	cmp	x0, 0
	bne	.L4
.L2:
	bl	fs_get_root_node
	str	x0, [sp, 352]
	ldr	x0, [sp, 344]
	add	x0, x0, 1
	str	x0, [sp, 344]
	b	.L4
.L16:
	ldr	x1, [sp, 48]
	ldr	x0, [sp, 344]
	add	x3, x1, x0
	mov	w2, 2
	adrp	x0, .LC0
	add	x1, x0, :lo12:.LC0
	mov	x0, x3
	bl	utils_strncmp
	cmp	w0, 0
	bne	.L5
	ldr	x0, [sp, 352]
	ldr	x0, [x0, 64]
	cmp	x0, 0
	bne	.L6
	adrp	x0, .LC1
	add	x0, x0, :lo12:.LC1
	bl	tfp_printf
	mov	w0, -1
	b	.L18
.L6:
	ldr	x0, [sp, 352]
	ldr	x0, [x0, 64]
	str	x0, [sp, 352]
	ldr	x0, [sp, 344]
	add	x0, x0, 2
	str	x0, [sp, 344]
	b	.L4
.L5:
	ldr	x1, [sp, 48]
	ldr	x0, [sp, 344]
	add	x3, x1, x0
	mov	w2, 1
	adrp	x0, .LC2
	add	x1, x0, :lo12:.LC2
	mov	x0, x3
	bl	utils_strncmp
	cmp	w0, 0
	bne	.L8
	ldr	x0, [sp, 344]
	add	x0, x0, 1
	str	x0, [sp, 344]
	b	.L4
.L8:
	ldr	x1, [sp, 48]
	ldr	x0, [sp, 344]
	add	x3, x1, x0
	mov	w2, 1
	adrp	x0, .LC3
	add	x1, x0, :lo12:.LC3
	mov	x0, x3
	bl	utils_strncmp
	cmp	w0, 0
	bne	.L14
	ldr	x0, [sp, 360]
	cmp	x0, 0
	bne	.L10
	ldr	x0, [sp, 344]
	add	x0, x0, 1
	str	x0, [sp, 344]
	b	.L4
.L10:
	ldr	x0, [sp, 360]
	add	x1, sp, 80
	strb	wzr, [x1, x0]
	ldr	x0, [sp, 352]
	ldr	x0, [x0, 8]
	ldr	x3, [x0]
	add	x1, sp, 80
	add	x0, sp, 64
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [sp, 352]
	blr	x3
	str	w0, [sp, 336]
	ldr	w0, [sp, 336]
	cmp	w0, 0
	beq	.L11
	add	x0, sp, 80
	ldr	x2, [sp, 48]
	mov	x1, x0
	adrp	x0, .LC4
	add	x0, x0, :lo12:.LC4
	bl	tfp_printf
	ldr	w0, [sp, 336]
	b	.L18
.L11:
	ldr	x0, [sp, 64]
	ldr	w0, [x0, 80]
	and	w0, w0, 61440
	cmp	w0, 16384
	beq	.L13
	add	x0, sp, 80
	ldr	x2, [sp, 48]
	mov	x1, x0
	adrp	x0, .LC5
	add	x0, x0, :lo12:.LC5
	bl	tfp_printf
	mov	w0, -1
	b	.L18
.L13:
	ldr	x0, [sp, 64]
	str	x0, [sp, 352]
	ldr	x0, [sp, 344]
	add	x0, x0, 1
	str	x0, [sp, 344]
	str	xzr, [sp, 360]
	b	.L4
.L15:
	ldr	x0, [sp, 344]
	add	x1, x0, 1
	str	x1, [sp, 344]
	ldr	x1, [sp, 48]
	add	x1, x1, x0
	ldr	x0, [sp, 360]
	add	x2, x0, 1
	str	x2, [sp, 360]
	ldrb	w2, [x1]
	add	x1, sp, 80
	strb	w2, [x1, x0]
.L14:
	ldr	x1, [sp, 48]
	ldr	x0, [sp, 344]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 47
	beq	.L4
	ldr	x1, [sp, 48]
	ldr	x0, [sp, 344]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L15
.L4:
	ldr	x1, [sp, 48]
	ldr	x0, [sp, 344]
	add	x0, x1, x0
	ldrb	w0, [x0]
	cmp	w0, 0
	bne	.L16
	ldr	x0, [sp, 360]
	add	x1, sp, 80
	strb	wzr, [x1, x0]
	add	x0, sp, 80
	ldr	x2, [sp, 360]
	ldr	x1, [sp, 24]
	bl	memcpy
	ldr	x0, [sp, 352]
	ldr	x0, [x0, 8]
	ldr	x3, [x0]
	add	x1, sp, 80
	add	x0, sp, 72
	mov	x2, x1
	mov	x1, x0
	ldr	x0, [sp, 352]
	blr	x3
	str	w0, [sp, 340]
	ldr	x0, [sp, 40]
	ldr	x1, [sp, 352]
	str	x1, [x0]
	ldr	w0, [sp, 340]
	cmp	w0, 0
	beq	.L17
	mov	w0, -10
	b	.L18
.L17:
	ldr	x1, [sp, 72]
	ldr	x0, [sp, 32]
	str	x1, [x0]
	mov	w0, 0
.L18:
	ldp	x29, x30, [sp], 368
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE0:
	.size	fs_find_node, .-fs_find_node
	.align	2
	.global	vfs_lookup
	.type	vfs_lookup, %function
vfs_lookup:
.LFB1:
	.cfi_startproc
	stp	x29, x30, [sp, -320]!
	.cfi_def_cfa_offset 320
	.cfi_offset 29, -320
	.cfi_offset 30, -312
	mov	x29, sp
	str	x0, [sp, 40]
	str	x1, [sp, 32]
	str	x2, [sp, 24]
	str	xzr, [sp, 312]
	str	xzr, [sp, 304]
	add	x2, sp, 48
	add	x1, sp, 304
	add	x0, sp, 312
	mov	x4, x2
	mov	x3, x1
	mov	x2, x0
	ldr	x1, [sp, 32]
	ldr	x0, [sp, 40]
	bl	fs_find_node
	ldr	x0, [sp, 304]
	cmp	x0, 0
	beq	.L20
	ldr	x1, [sp, 304]
	ldr	x0, [sp, 24]
	str	x1, [x0]
	mov	w0, 0
	b	.L22
.L20:
	mov	w0, -1
.L22:
	ldp	x29, x30, [sp], 320
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE1:
	.size	vfs_lookup, .-vfs_lookup
	.section	.rodata
	.align	3
.LC6:
	.string	"node name: %s\n"
	.text
	.align	2
	.global	vfs_mkdir
	.type	vfs_mkdir, %function
vfs_mkdir:
.LFB2:
	.cfi_startproc
	stp	x29, x30, [sp, -320]!
	.cfi_def_cfa_offset 320
	.cfi_offset 29, -320
	.cfi_offset 30, -312
	mov	x29, sp
	str	x0, [sp, 24]
	str	x1, [sp, 16]
	str	xzr, [sp, 304]
	str	xzr, [sp, 296]
	add	x2, sp, 40
	add	x1, sp, 296
	add	x0, sp, 304
	mov	x4, x2
	mov	x3, x1
	mov	x2, x0
	ldr	x1, [sp, 16]
	ldr	x0, [sp, 24]
	bl	fs_find_node
	str	w0, [sp, 316]
	ldr	w0, [sp, 316]
	cmn	w0, #10
	beq	.L24
	ldr	w0, [sp, 316]
	b	.L26
.L24:
	add	x0, sp, 40
	mov	x1, x0
	adrp	x0, .LC6
	add	x0, x0, :lo12:.LC6
	bl	tfp_printf
	ldr	x0, [sp, 304]
	ldr	x0, [x0, 8]
	ldr	x3, [x0, 16]
	ldr	x0, [sp, 304]
	add	x2, sp, 40
	add	x1, sp, 296
	blr	x3
.L26:
	ldp	x29, x30, [sp], 320
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE2:
	.size	vfs_mkdir, .-vfs_mkdir
	.section	.rodata
	.align	3
.LC7:
	.string	"[FS][TRACE] fs_init() started.\n"
	.align	3
.LC8:
	.string	"[FS][ERROR] Failed to register rootfs\n"
	.align	3
.LC9:
	.string	"rootfs"
	.align	3
.LC10:
	.string	"[FS][ERROR] Failed to get root file system.\n"
	.align	3
.LC11:
	.string	""
	.align	3
.LC12:
	.string	"/initramfs"
	.align	3
.LC13:
	.string	"[FS][ERROR] Failed to make /initramfs directory\n"
	.align	3
.LC14:
	.string	"[FS][ERROR] Failed to register initramfs\n"
	.align	3
.LC15:
	.string	"initramfs"
	.align	3
.LC16:
	.string	"[FS][ERROR] Failed to get cpio file system.\n"
	.align	3
.LC17:
	.string	"[FS][ERROR] Failed to get /initramfs directory\n"
	.align	3
.LC18:
	.string	"[FS][ERROR] Failed to mount initramfs.\n"
	.align	3
.LC19:
	.string	"/dev"
	.align	3
.LC20:
	.string	"[FS][ERROR] Failed to make /dev directory\n"
	.align	3
.LC21:
	.string	"[FS][ERROR] Failed to register uartfs\n"
	.align	3
.LC22:
	.string	"uartfs"
	.align	3
.LC23:
	.string	"[FS][ERROR] Failed to get uart file system.\n"
	.align	3
.LC24:
	.string	"/dev/uart"
	.align	3
.LC25:
	.string	"[FS][ERROR] Failed to make /dev/uart directory\n"
	.align	3
.LC26:
	.string	"[FS][ERROR] Failed to get /dev/uart directory\n"
	.align	3
.LC27:
	.string	"[FS][ERROR] Failed to mount uartfs.\n"
	.align	3
.LC28:
	.string	"[FS][TRACE] fs_init() success.\n"
	.text
	.align	2
	.global	fs_init
	.type	fs_init, %function
fs_init:
.LFB3:
	.cfi_startproc
	stp	x29, x30, [sp, -80]!
	.cfi_def_cfa_offset 80
	.cfi_offset 29, -80
	.cfi_offset 30, -72
	mov	x29, sp
	str	x19, [sp, 16]
	.cfi_offset 19, -64
	adrp	x0, .LC7
	add	x0, x0, :lo12:.LC7
	bl	tfp_printf
	mov	x0, 24
	bl	kzalloc
	mov	x1, x0
	adrp	x0, fs_manager
	add	x0, x0, :lo12:fs_manager
	str	x1, [x0]
	adrp	x0, fs_manager
	add	x0, x0, :lo12:fs_manager
	ldr	x0, [x0]
	add	x0, x0, 8
	bl	link_list_init
	mov	w0, -1
	str	w0, [sp, 76]
	bl	rootfs_create
	bl	fs_register
	cmp	w0, 0
	beq	.L28
	adrp	x0, .LC8
	add	x0, x0, :lo12:.LC8
	bl	tfp_printf
	b	.L27
.L28:
	adrp	x0, .LC9
	add	x0, x0, :lo12:.LC9
	bl	fs_get
	str	x0, [sp, 64]
	ldr	x0, [sp, 64]
	cmp	x0, 0
	bne	.L30
	adrp	x0, .LC10
	add	x0, x0, :lo12:.LC10
	bl	tfp_printf
	b	.L27
.L30:
	adrp	x0, fs_manager
	add	x0, x0, :lo12:fs_manager
	ldr	x0, [x0]
	ldr	x0, [x0]
	ldr	x1, [sp, 64]
	str	x1, [x0, 8]
	adrp	x0, fs_manager
	add	x0, x0, :lo12:fs_manager
	ldr	x0, [x0]
	ldr	x19, [x0]
	mov	w1, 16384
	adrp	x0, .LC11
	add	x0, x0, :lo12:.LC11
	bl	vnode_create
	str	x0, [x19]
	adrp	x0, fs_manager
	add	x0, x0, :lo12:fs_manager
	ldr	x0, [x0]
	ldr	x0, [x0]
	ldr	x0, [x0, 8]
	ldr	x2, [x0, 8]
	adrp	x0, fs_manager
	add	x0, x0, :lo12:fs_manager
	ldr	x0, [x0]
	ldr	x0, [x0]
	mov	x1, x0
	ldr	x0, [sp, 64]
	blr	x2
	adrp	x0, .LC12
	add	x1, x0, :lo12:.LC12
	mov	x0, 0
	bl	vfs_mkdir
	str	w0, [sp, 76]
	ldr	w0, [sp, 76]
	cmp	w0, 0
	beq	.L31
	adrp	x0, .LC13
	add	x0, x0, :lo12:.LC13
	bl	tfp_printf
	b	.L27
.L31:
	bl	initramfs_create
	bl	fs_register
	cmp	w0, 0
	beq	.L32
	adrp	x0, .LC14
	add	x0, x0, :lo12:.LC14
	bl	tfp_printf
	b	.L27
.L32:
	adrp	x0, .LC15
	add	x0, x0, :lo12:.LC15
	bl	fs_get
	str	x0, [sp, 64]
	ldr	x0, [sp, 64]
	cmp	x0, 0
	bne	.L33
	adrp	x0, .LC16
	add	x0, x0, :lo12:.LC16
	bl	tfp_printf
	b	.L27
.L33:
	str	xzr, [sp, 48]
	add	x0, sp, 48
	mov	x2, x0
	adrp	x0, .LC12
	add	x1, x0, :lo12:.LC12
	mov	x0, 0
	bl	vfs_lookup
	str	w0, [sp, 76]
	ldr	w0, [sp, 76]
	cmp	w0, 0
	beq	.L34
	adrp	x0, .LC17
	add	x0, x0, :lo12:.LC17
	bl	tfp_printf
	b	.L27
.L34:
	mov	x0, 16
	bl	kzalloc
	str	x0, [sp, 56]
	ldr	x0, [sp, 56]
	ldr	x1, [sp, 64]
	str	x1, [x0, 8]
	ldr	x1, [sp, 48]
	ldr	x0, [sp, 56]
	str	x1, [x0]
	ldr	x0, [sp, 64]
	ldr	x2, [x0, 8]
	ldr	x1, [sp, 56]
	ldr	x0, [sp, 64]
	blr	x2
	str	w0, [sp, 76]
	ldr	w0, [sp, 76]
	cmp	w0, 0
	beq	.L35
	adrp	x0, .LC18
	add	x0, x0, :lo12:.LC18
	bl	tfp_printf
	b	.L27
.L35:
	adrp	x0, .LC19
	add	x1, x0, :lo12:.LC19
	mov	x0, 0
	bl	vfs_mkdir
	str	w0, [sp, 76]
	ldr	w0, [sp, 76]
	cmp	w0, 0
	beq	.L36
	adrp	x0, .LC20
	add	x0, x0, :lo12:.LC20
	bl	tfp_printf
	b	.L27
.L36:
	bl	uartfs_create
	bl	fs_register
	cmp	w0, 0
	beq	.L37
	adrp	x0, .LC21
	add	x0, x0, :lo12:.LC21
	bl	tfp_printf
	b	.L27
.L37:
	adrp	x0, .LC22
	add	x0, x0, :lo12:.LC22
	bl	fs_get
	str	x0, [sp, 64]
	ldr	x0, [sp, 64]
	cmp	x0, 0
	bne	.L38
	adrp	x0, .LC23
	add	x0, x0, :lo12:.LC23
	bl	tfp_printf
	b	.L27
.L38:
	adrp	x0, .LC24
	add	x1, x0, :lo12:.LC24
	mov	x0, 0
	bl	vfs_mkdir
	cmp	w0, 0
	beq	.L39
	adrp	x0, .LC25
	add	x0, x0, :lo12:.LC25
	bl	tfp_printf
.L39:
	str	xzr, [sp, 40]
	add	x0, sp, 40
	mov	x2, x0
	adrp	x0, .LC24
	add	x1, x0, :lo12:.LC24
	mov	x0, 0
	bl	vfs_lookup
	cmp	w0, 0
	beq	.L40
	adrp	x0, .LC26
	add	x0, x0, :lo12:.LC26
	bl	tfp_printf
.L40:
	mov	x0, 16
	bl	kzalloc
	str	x0, [sp, 56]
	ldr	x0, [sp, 56]
	ldr	x1, [sp, 64]
	str	x1, [x0, 8]
	ldr	x1, [sp, 40]
	ldr	x0, [sp, 56]
	str	x1, [x0]
	ldr	x0, [sp, 64]
	ldr	x2, [x0, 8]
	ldr	x1, [sp, 56]
	ldr	x0, [sp, 64]
	blr	x2
	cmp	w0, 0
	beq	.L41
	adrp	x0, .LC27
	add	x0, x0, :lo12:.LC27
	bl	tfp_printf
	b	.L27
.L41:
	adrp	x0, .LC28
	add	x0, x0, :lo12:.LC28
	bl	tfp_printf
.L27:
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 80
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 19
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE3:
	.size	fs_init, .-fs_init
	.align	2
	.global	fs_get_root_node
	.type	fs_get_root_node, %function
fs_get_root_node:
.LFB4:
	.cfi_startproc
	adrp	x0, fs_manager
	add	x0, x0, :lo12:fs_manager
	ldr	x0, [x0]
	ldr	x0, [x0]
	ldr	x0, [x0]
	ret
	.cfi_endproc
.LFE4:
	.size	fs_get_root_node, .-fs_get_root_node
	.align	2
	.global	fs_register
	.type	fs_register, %function
fs_register:
.LFB5:
	.cfi_startproc
	stp	x29, x30, [sp, -32]!
	.cfi_def_cfa_offset 32
	.cfi_offset 29, -32
	.cfi_offset 30, -24
	mov	x29, sp
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	ldr	x0, [x0]
	bl	fs_get
	cmp	x0, 0
	bne	.L46
	adrp	x0, fs_manager
	add	x0, x0, :lo12:fs_manager
	ldr	x0, [x0]
	add	x2, x0, 8
	ldr	x0, [sp, 24]
	add	x0, x0, 16
	mov	x1, x0
	mov	x0, x2
	bl	link_list_push_back
	mov	w0, 0
	b	.L47
.L46:
	mov	w0, -1
.L47:
	ldp	x29, x30, [sp], 32
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE5:
	.size	fs_register, .-fs_register
	.section	.rodata
	.align	3
.LC29:
	.string	"[FS] vfs_open(): opening %s\n"
	.text
	.align	2
	.global	vfs_open
	.type	vfs_open, %function
vfs_open:
.LFB6:
	.cfi_startproc
	stp	x29, x30, [sp, -336]!
	.cfi_def_cfa_offset 336
	.cfi_offset 29, -336
	.cfi_offset 30, -328
	mov	x29, sp
	str	x0, [sp, 40]
	str	x1, [sp, 32]
	str	w2, [sp, 28]
	str	x3, [sp, 16]
	ldr	x1, [sp, 32]
	adrp	x0, .LC29
	add	x0, x0, :lo12:.LC29
	bl	tfp_printf
	str	xzr, [sp, 312]
	str	xzr, [sp, 304]
	add	x2, sp, 48
	add	x1, sp, 304
	add	x0, sp, 312
	mov	x4, x2
	mov	x3, x1
	mov	x2, x0
	ldr	x1, [sp, 32]
	ldr	x0, [sp, 40]
	bl	fs_find_node
	str	w0, [sp, 332]
	ldr	w0, [sp, 332]
	cmp	w0, 0
	beq	.L49
	ldr	w0, [sp, 28]
	and	w0, w0, 4
	cmp	w0, 0
	bne	.L49
	ldr	w0, [sp, 332]
	b	.L53
.L49:
	ldr	x0, [sp, 304]
	cmp	x0, 0
	bne	.L51
	ldr	w0, [sp, 332]
	cmn	w0, #10
	bne	.L51
	ldr	x0, [sp, 312]
	ldr	x0, [x0, 8]
	ldr	x3, [x0, 8]
	ldr	x0, [sp, 312]
	add	x2, sp, 48
	add	x1, sp, 304
	blr	x3
	str	w0, [sp, 332]
	ldr	w0, [sp, 332]
	cmp	w0, 0
	beq	.L51
	ldr	w0, [sp, 332]
	b	.L53
.L51:
	ldr	w0, [sp, 332]
	cmp	w0, 0
	beq	.L52
	ldr	w0, [sp, 332]
	b	.L53
.L52:
	mov	x0, 24
	bl	kzalloc
	str	x0, [sp, 320]
	ldr	x1, [sp, 304]
	ldr	x0, [sp, 320]
	str	x1, [x0]
	ldr	x0, [sp, 320]
	ldr	w1, [sp, 28]
	str	w1, [x0, 16]
	ldr	x0, [sp, 320]
	str	xzr, [x0, 8]
	ldr	x0, [sp, 16]
	ldr	x1, [sp, 320]
	str	x1, [x0]
	mov	w0, 0
.L53:
	ldp	x29, x30, [sp], 336
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE6:
	.size	vfs_open, .-vfs_open
	.section	.rodata
	.align	3
.LC30:
	.string	"[FS][ERROR] vfs_close(): failed to close file: %s\n"
	.text
	.align	2
	.global	vfs_close
	.type	vfs_close, %function
vfs_close:
.LFB7:
	.cfi_startproc
	stp	x29, x30, [sp, -32]!
	.cfi_def_cfa_offset 32
	.cfi_offset 29, -32
	.cfi_offset 30, -24
	mov	x29, sp
	str	x0, [sp, 24]
	ldr	x0, [sp, 24]
	cmp	x0, 0
	bne	.L55
	mov	w0, -1
	b	.L56
.L55:
	ldr	x0, [sp, 24]
	ldr	x0, [x0]
	ldr	x0, [x0, 16]
	ldr	x1, [x0, 24]
	ldr	x0, [sp, 24]
	blr	x1
	cmp	w0, 0
	beq	.L57
	ldr	x0, [sp, 24]
	ldr	x0, [x0]
	ldr	x0, [x0, 72]
	mov	x1, x0
	adrp	x0, .LC30
	add	x0, x0, :lo12:.LC30
	bl	tfp_printf
.L57:
	ldr	x0, [sp, 24]
	bl	kfree
	mov	w0, 0
.L56:
	ldp	x29, x30, [sp], 32
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE7:
	.size	vfs_close, .-vfs_close
	.align	2
	.global	vfs_write
	.type	vfs_write, %function
vfs_write:
.LFB8:
	.cfi_startproc
	stp	x29, x30, [sp, -48]!
	.cfi_def_cfa_offset 48
	.cfi_offset 29, -48
	.cfi_offset 30, -40
	mov	x29, sp
	str	x0, [sp, 40]
	str	x1, [sp, 32]
	str	x2, [sp, 24]
	ldr	x0, [sp, 40]
	ldr	x0, [x0]
	ldr	x0, [x0, 16]
	ldr	x3, [x0]
	ldr	x2, [sp, 24]
	ldr	x1, [sp, 32]
	ldr	x0, [sp, 40]
	blr	x3
	ldp	x29, x30, [sp], 48
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE8:
	.size	vfs_write, .-vfs_write
	.align	2
	.global	vfs_read
	.type	vfs_read, %function
vfs_read:
.LFB9:
	.cfi_startproc
	stp	x29, x30, [sp, -48]!
	.cfi_def_cfa_offset 48
	.cfi_offset 29, -48
	.cfi_offset 30, -40
	mov	x29, sp
	str	x0, [sp, 40]
	str	x1, [sp, 32]
	str	x2, [sp, 24]
	ldr	x0, [sp, 40]
	ldr	w0, [x0, 16]
	and	w0, w0, 1
	cmp	w0, 0
	bne	.L61
	mov	w0, -1
	b	.L62
.L61:
	ldr	x0, [sp, 40]
	ldr	x0, [x0]
	ldr	x0, [x0, 16]
	ldr	x3, [x0, 8]
	ldr	x2, [sp, 24]
	ldr	x1, [sp, 32]
	ldr	x0, [sp, 40]
	blr	x3
.L62:
	ldp	x29, x30, [sp], 48
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE9:
	.size	vfs_read, .-vfs_read
	.align	2
	.global	vfs_lseek64
	.type	vfs_lseek64, %function
vfs_lseek64:
.LFB10:
	.cfi_startproc
	stp	x29, x30, [sp, -48]!
	.cfi_def_cfa_offset 48
	.cfi_offset 29, -48
	.cfi_offset 30, -40
	mov	x29, sp
	str	x0, [sp, 40]
	str	x1, [sp, 32]
	str	w2, [sp, 28]
	ldr	x0, [sp, 40]
	ldr	x0, [x0]
	ldr	x0, [x0, 16]
	ldr	x3, [x0, 32]
	ldr	w2, [sp, 28]
	ldr	x1, [sp, 32]
	ldr	x0, [sp, 40]
	blr	x3
	ldp	x29, x30, [sp], 48
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE10:
	.size	vfs_lseek64, .-vfs_lseek64
	.align	2
	.global	fs_get
	.type	fs_get, %function
fs_get:
.LFB11:
	.cfi_startproc
	stp	x29, x30, [sp, -80]!
	.cfi_def_cfa_offset 80
	.cfi_offset 29, -80
	.cfi_offset 30, -72
	mov	x29, sp
	str	x19, [sp, 16]
	.cfi_offset 19, -64
	str	x0, [sp, 40]
	adrp	x0, fs_manager
	add	x0, x0, :lo12:fs_manager
	ldr	x0, [x0]
	ldr	x0, [x0, 16]
	str	x0, [sp, 64]
	ldr	x0, [sp, 64]
	sub	x0, x0, #16
	str	x0, [sp, 72]
	b	.L66
.L69:
	ldr	x0, [sp, 72]
	ldr	x19, [x0]
	ldr	x0, [sp, 40]
	bl	utils_strlen
	mov	w2, w0
	ldr	x1, [sp, 40]
	mov	x0, x19
	bl	utils_strncmp
	cmp	w0, 0
	bne	.L67
	ldr	x0, [sp, 72]
	b	.L68
.L67:
	ldr	x0, [sp, 72]
	ldr	x0, [x0, 24]
	str	x0, [sp, 56]
	ldr	x0, [sp, 56]
	sub	x0, x0, #16
	str	x0, [sp, 72]
.L66:
	ldr	x0, [sp, 72]
	add	x1, x0, 16
	adrp	x0, fs_manager
	add	x0, x0, :lo12:fs_manager
	ldr	x0, [x0]
	add	x0, x0, 8
	cmp	x1, x0
	bne	.L69
	mov	x0, 0
.L68:
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 80
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 19
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE11:
	.size	fs_get, .-fs_get
	.align	2
	.global	vnode_create
	.type	vnode_create, %function
vnode_create:
.LFB12:
	.cfi_startproc
	stp	x29, x30, [sp, -48]!
	.cfi_def_cfa_offset 48
	.cfi_offset 29, -48
	.cfi_offset 30, -40
	mov	x29, sp
	str	x0, [sp, 24]
	str	w1, [sp, 20]
	mov	x0, 104
	bl	kzalloc
	str	x0, [sp, 40]
	ldr	x0, [sp, 40]
	add	x0, x0, 24
	bl	link_list_init
	ldr	x0, [sp, 40]
	add	x0, x0, 48
	bl	link_list_init
	ldr	x0, [sp, 40]
	str	xzr, [x0, 40]
	ldr	x0, [sp, 40]
	str	xzr, [x0, 64]
	ldr	x0, [sp, 24]
	bl	utils_strlen
	str	x0, [sp, 32]
	mov	x0, 8
	bl	kzalloc
	mov	x1, x0
	ldr	x0, [sp, 40]
	str	x1, [x0, 72]
	ldr	x0, [sp, 40]
	ldr	x0, [x0, 72]
	ldr	x2, [sp, 32]
	mov	x1, x0
	ldr	x0, [sp, 24]
	bl	memcpy
	ldr	x0, [sp, 40]
	ldr	w1, [sp, 20]
	str	w1, [x0, 80]
	ldr	x0, [sp, 40]
	str	xzr, [x0, 88]
	ldr	x0, [sp, 40]
	str	xzr, [x0, 96]
	ldr	x0, [sp, 40]
	ldp	x29, x30, [sp], 48
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE12:
	.size	vnode_create, .-vnode_create
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
