.section ".text.boot"

.global _start

_start:

/* get cpu id */
//mrs x1, mpidr_el1
//and x1, x1, #3
//cbnz x1, halt // halt if cpu id != 0

  bl from_el2_to_el1

/* set stack pointer */
  ldr x1, =_start
  mov sp, x1
//msr SP_EL1, x1

/* clear bss section */
  ldr x1, =bss_begin
  ldr w2, =bss_size

bss_init:
  cbz w2, run_main
  str xzr, [x1], #8
  sub w2, w2, #1
  cbnz w2, bss_init

run_main:
/* branch to main function */
  bl main

halt:
  wfe
  b halt

from_el2_to_el1:
  mov x4, (1<<31) // EL1 uses aarch64
  msr hcr_el2, x4 // hcr_el2's bit31 is RW: 0->lower levels are all aarch32; 1->EL1: aarch64,
// EL0 excution state determined by PSTATE.nRW value
  mov x4, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
  msr spsr_el2, x4 // 001111000101, spsr_el2 bit[9:6] are D, A, I, and F respectly,
// bit[3:0]: select stack pointer:0101->el1 with sp_el1
  msr elr_el2, lr
  eret // return to EL1
