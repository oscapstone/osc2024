#include "mmu_regs.h"

#define CORE0_TIMER_IRQ_CTRL 0xffff000040000040

.section ".text.boot"

.globl _start
_start:
    mov x19, x0

	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF	// Check processor id
	cbz	x0, setup_tcr_el1		// Hang for all non-primary CPU
	b	proc_hang

proc_hang: 
	b 	proc_hang

setup_tcr_el1:
	bl from_el2_to_el1

    ldr x0, = TCR_CONFIG_DEFAULT
    msr tcr_el1, x0

    ldr x0, =( \
        (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
        (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) \
    )
    msr mair_el1, x0

    mov x0, 0x1000 // PGD's page frame at 0x1000
    mov x1, 0x2000 // PUD's page frame at 0x2000

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

    mov sp, 0x100000
    bl three_level_translation_init

    mrs x2, sctlr_el1
    orr x2 , x2, 1
    msr sctlr_el1, x2 // enable MMU, cache remains disabled

    ldr x2,= boot_rest
    br x2

boot_rest:

	
	ldr x1, =0xffff000000100000
	mov sp, x1
	
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	bl set_exception_vector_table
	
	// bl from_el1_to_el0

	// bl core_timer_enable

	msr daifclr, 0xf 	// enable el1 interrupt

	mov x0, x19         // get fdt back
	bl  kernel_begin
	b 	proc_hang		// should never come here


from_el2_to_el1:
  mov x0, (1 << 31)
  msr hcr_el2, x0
  mov x0, 5 
  msr spsr_el2, x0
  msr elr_el2, lr
  eret

// save general registers to stack
.macro save_all
    sub sp, sp, 32 * 10 
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]
	// empty 8 bytes

	mrs x0, SPSR_EL1
	mrs x1, ELR_EL1
	stp x0, x1, [sp, 16 * 16]
	mrs x0, sp_el0
	str x0, [sp, 16 * 17]
	// nested shit

.endm

// load general registers from stack
.macro load_all
	ldp x0, x1,  [sp, 16 * 16]
	msr SPSR_EL1, x0
	msr ELR_EL1,  x1
	ldr x0, [sp, 16 * 17]
	msr sp_el0, x0

	ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]

    add sp, sp, 32 * 10
.endm

exception_handler:
  save_all
  mov x0, sp
  bl exception_entry 
  load_all
  eret

.global core_timer_enable
core_timer_enable:
  mov x0, 1
  msr cntp_ctl_el0, x0
  // mrs x0, cntfrq_el0
  mov x0, 0 
  msr cntp_tval_el0, x0
  mov x0, 2
  ldr x1, =CORE0_TIMER_IRQ_CTRL
  str w0, [x1]
  mrs x0, cntkctl_el1
  orr x0, x0, #1
  msr cntkctl_el1, x0

  ret

core_timer_handler:
  save_all
  bl core_timer_entry 
  load_all
  eret

general_irq_handler:
  save_all
  // mov x0, sp
  // bl debug
  bl c_general_irq_handler
  // mov x0, sp
  // bl debug
  // bl handle_signal
  load_all
  eret

system_call_handler:
	cmp x8, #10
	beq system_call_handler_end
	save_all
	mov x0, sp
	bl c_system_call_handler
system_call_handler_end:
	load_all
	eret

sig_return_handler:
	load_all
	eret

undefined_exception:
	bl c_undefined_exception

.align 11
.global exception_vector_table
exception_vector_table:
  b undefined_exception 
  .align 7
  b undefined_exception 
  .align 7
  b undefined_exception
  .align 7
  b undefined_exception
  .align 7

  b system_call_handler 
  .align 7
  b general_irq_handler 
  .align 7
  b undefined_exception
  .align 7
  b undefined_exception
  .align 7

  b system_call_handler 
  .align 7
  b general_irq_handler 
  .align 7
  b undefined_exception
  .align 7
  b undefined_exception
  .align 7

  b undefined_exception
  .align 7
  b undefined_exception
  .align 7
  b undefined_exception
  .align 7
  b undefined_exception
  .align 7

set_exception_vector_table:
  	adr x0, exception_vector_table 
	msr vbar_el1, x0
	ret

/*
.global to_el0_with_timer
to_el0_with_timer:
	mov x29, lr
	bl from_el1_to_el0
	bl core_timer_enable
	b proc_hang
	mov lr, x29
	ret
*/

.global switch_to
switch_to:
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]

	ldp x19, x20, [x1, 16 * 0]
    ldp x21, x22, [x1, 16 * 1] // x0, x1 are 0xffff shits
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]
    ldr x9, [x1, 16 * 6]
    mov sp,  x9
    msr tpidr_el1, x1

	ldr x2, [x1, 16 * 6 + 8]
	dsb ish // ensure write has completed
    msr ttbr0_el1, x2 // switch translation based address.
    tlbi vmalle1is // invalidate all TLB entries
    dsb ish // ensure completion of TLB invalidatation
    isb // clear pipeline

    ret

.global get_current
get_current:
    mrs x0, tpidr_el1
    ret

.global ret_from_fork_child
ret_from_fork_child:
	load_all
	// bl im_fineee
	// msr DAIFclr, 0xf
	eret
