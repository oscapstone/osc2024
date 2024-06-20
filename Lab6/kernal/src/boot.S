#include "mmu.h"

.section ".text.boot"

.globl _start

_start:

	mov     x10, x0
	mov     x11, x1
	mov     x12, x2
	mov     x13, x3

_check_proc:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b	proc_hang

proc_hang: 
	b 	proc_hang
 
master:

	bl from_el2_to_el1

setup_mmu:

	ldr x0, = TCR_CONFIG_DEFAULT
	msr tcr_el1, x0
	ldr x0, =( \
	(MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
	(MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) \
	)
	msr mair_el1, x0

	mov x0, KERNEL_PGD_BASE // PGD's page frame
	mov x1, KERNEL_PUD_BASE // PUD's page frame

	ldr x2, = BOOT_PGD_ATTR
	orr x2, x1, x2 // combine the physical address of next level page with attribute.
	str x2, [x0]

	ldr x2, = BOOT_PUD_ATTR
	mov x3, 0x00000000
	orr x3, x2, x3
	str x3, [x1] // 1st 1GB mapped by the 1st entry of PUD
	mov x3, 0x40000000
	orr x3, x2, x3
	str x3, [x1, 8] // 2nd 1GB mapped by the 2nd entry of PUD

	msr ttbr0_el1, x0 // load PGD to the bottom translation-based register.
	msr ttbr1_el1, x0 // also load PGD to the upper translation based register.

	mov sp, 0x3c000000 //temp stack
	bl set_2M_kernel_mmu

	mrs x2, sctlr_el1
	orr x2 , x2, 1
	msr sctlr_el1, x2 // enable MMU, cache remains disabled

	ldr x2, = boot_rest // indirect branch to the virtual address
	br x2

boot_rest:
	mov x0, 0xffffffff
	msr ttbr0_el1, x0
	
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	ldr x1, =_stack_top
    	mov sp, x1
    	
    	
    bl core_timer_enable
    	
	mov     x0, x10
	mov     x1, x11
	mov     x2, x12
	mov     x3, x13

	bl	kernel_main
	b 	proc_hang		// should never come here

memzero:
	str xzr, [x0], #8
	subs x1, x1, #8
	b.gt memzero
	ret
	
from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    //mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    mov x0, 0x345
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1
