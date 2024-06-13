.section ".text.exception"

#define SVC_64 0b010101
#define DATA_ABORT_FROM_LOWER_LV 0b100100
#define INSTRUCTION_ABORT_FROM_LOWER_LV 0b100000

// save general registers to stack
.macro save_all
    sub sp, sp, #272
    stp x0, x1, [sp, #16 * 0]
    stp x2, x3, [sp, #16 * 1]
    stp x4, x5, [sp, #16 * 2]
    stp x6, x7, [sp, #16 * 3]
    stp x8, x9, [sp, #16 * 4]
    stp x10, x11, [sp, #16 * 5]
    stp x12, x13, [sp, #16 * 6]
    stp x14, x15, [sp, #16 * 7]
    stp x16, x17, [sp, #16 * 8]
    stp x18, x19, [sp, #16 * 9]
    stp x20, x21, [sp, #16 * 10]
    stp x22, x23, [sp, #16 * 11]
    stp x24, x25, [sp, #16 * 12]
    stp x26, x27, [sp, #16 * 13]
    stp x28, x29, [sp, #16 * 14]
    mrs x21, sp_el0
    stp x30, x21, [sp, #16 * 15]
    mrs x21, elr_el1
    mrs x22, spsr_el1
    stp x21, x22, [sp, #16 * 16]
.endm

// load general registers from stack
.macro load_all
    ldp x21, x22, [sp, #16 * 16]
    msr spsr_el1, x22
    msr elr_el1, x21
    ldp x30, x21, [sp, #16 * 15]
    msr sp_el0, x21
    ldp x28, x29, [sp, #16 * 14]
    ldp x26, x27, [sp, #16 * 13]
    ldp x24, x25, [sp, #16 * 12]
    ldp x22, x23, [sp, #16 * 11]
    ldp x20, x21, [sp, #16 * 10]
    ldp x18, x19, [sp, #16 * 9]
    ldp x16, x17, [sp, #16 * 8]
    ldp x14, x15, [sp, #16 * 7]
    ldp x12, x13, [sp, #16 * 6]
    ldp x10, x11, [sp, #16 * 5]
    ldp x8, x9, [sp, #16 * 4]
    ldp x6, x7, [sp, #16 * 3]
    ldp x4, x5, [sp, #16 * 2]
    ldp x2, x3, [sp, #16 * 1]
    ldp x0, x1, [sp, #16 * 0]
    add sp, sp, #272
.endm

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
  b exception_handler // branch to a handler function.
  .align 7 // entry size is 0x80, .align will pad 0
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b el1_sync_handler
  .align 7
  b irq_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b el0_sync_handler
  .align 7
  b irq_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

exception_handler:
  save_all
  bl exception_entry
  load_all
  eret

el1_sync_handler:
  save_all
  bl el1_sync_handler_entry
  load_all
  eret

irq_handler:
  save_all
  bl irq_handler_entry
  bl check_need_schedule
  mov x0, sp // pass signal frame
  bl check_signal
  load_all
  eret

el0_sync_handler:
  save_all
  mrs x25, esr_el1 // read the syndrome register
  lsr	x24, x25, 26 // exception class
  cmp x24, SVC_64 // SVC in 64-bit state
  beq el0_svc
  cmp	x24, DATA_ABORT_FROM_LOWER_LV	// data abort in EL0
	beq	el0_da
  cmp	x24, INSTRUCTION_ABORT_FROM_LOWER_LV	// instruction abort in EL0
	beq	el0_ia
  bl el1_sync_handler_entry
  load_all
  eret


el0_svc:
  mov x0, sp // pass trap frame
  bl el0_svc_handler_entry
  mov x0, sp // pass signal frame
  bl check_signal
  load_all
  eret

el0_da:
  bl el0_da_handler_entry
  load_all
  eret

el0_ia:
  bl el0_ia_handler_entry
  load_all
  eret

.global return_from_fork
return_from_fork:
  load_all
  eret