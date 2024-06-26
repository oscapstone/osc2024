#include "mm.h"
#include "arm/sysregs.h"
#include "arm/mmu.h"
#include "peripheral/base.h"

.section ".text.boot"

.globl _start
_start:
    mov x10, x0 // protect the dtb address sent from the bootloader

    mrs x1, mpidr_el1
    and x1, x1, #3
    cbz x1, master
    b proc_hang

proc_hang:
    b proc_hang

/* Master core */
master:
    mrs x1, CurrentEL
    lsr x1, x1, #2

    // running at EL3?
    cmp x1, #3
    bne from_el2_to_el1

    ldr x1, =SCTLR_VALUE
    msr sctlr_el1, x1

    ldr x1, =HCR_VALUE
    msr hcr_el2, x1

    ldr x1, =SCR_VALUE
    msr scr_el3, x1

    ldr x1, =SPSR_VALUE
    msr spsr_el3, x1

    adr x1, el1_entry
    msr elr_el3, x1

    eret

from_el2_to_el1:
    ldr x1, =SCTLR_VALUE
    msr sctlr_el1, x1

    ldr x1, =HCR_VALUE
    msr hcr_el2, x1

    ldr x1, =SPSR_VALUE
    msr spsr_el2, x1

    adr x1, el1_entry
    msr elr_el2, x1

    eret

el1_entry:
    adr x0, bss_begin
    adr x1, bss_end
    sub x1, x1, x0
    bl memzero

    bl __create_page_tables

    mov x0, #VA_START
    add sp, x0, #LOW_MEMORY

    adrp x0, pg_dir
    msr ttbr1_el1, x0
    msr ttbr0_el1, x0

    ldr x0, =(TCR_VALUE)
    msr tcr_el1, x0

    ldr x0, =(MAIR_VALUE)
    msr mair_el1, x0

    ldr x2, =kernel_main
    mov x0, x10

    mrs x1, sctlr_el1
    orr x1, x1, #SCTLR_MMU_ENABLED
    msr sctlr_el1, x1

    br x2

    b proc_hang     // should never come here

.macro create_pgd_entry, tbl, virt, tmp1, tmp2
  add \tmp2, \tbl, #PAGE_SIZE
  create_table_entry \tbl, \virt, PGD_SHIFT, \tmp2, \tmp1
  add \tbl, \tbl, #PAGE_SIZE
.endm

.macro create_pud_entry, tbl, virt, tmp1, tmp2
  add \tmp2, \tbl, #PAGE_SIZE
  create_table_entry \tbl, \virt, PUD_SHIFT, \tmp2, \tmp1
  add \tmp2, \tmp2, #PAGE_SIZE
  mov \tmp1, #PHYS_MEMORY_SIZE
  add \virt, \virt, \tmp1
  create_table_entry \tbl, \virt, PUD_SHIFT, \tmp2, \tmp1
  add \tbl, \tbl, #PAGE_SIZE
.endm

.macro create_table_entry, tbl, virt, shift, next_tbl, tmp1
  lsr \tmp1, \virt, #\shift
  and \tmp1, \tmp1, #PTRS_PER_TABLE - 1   // table index
  // add \tmp2, \tbl, #PAGE_SIZE             // next level table address
  // orr \tmp2, \tmp2, #MM_TYPE_PAGE_TABLE
  orr \next_tbl, \next_tbl, #MM_TYPE_PAGE_TABLE
  str \next_tbl, [\tbl, \tmp1, lsl #3]
  // add \tbl, \tbl, #PAGE_SIZE              // next level table page
.endm


.macro create_block_map tbl, phys, start, end, flags, tmp1
  lsr \start, \start, #SECTION_SHIFT
  and \start, \start, #PTRS_PER_TABLE - 1     // table index

  lsr \end, \end, #SECTION_SHIFT
  and \end, \end, #PTRS_PER_TABLE - 1         // table end index

  lsr \phys, \phys, #SECTION_SHIFT
  mov \tmp1, #\flags
  orr \phys, \tmp1, \phys, lsl #SECTION_SHIFT // table entry

9999: str \phys, [\tbl, \start, lsl #3]       // store the entry
  add \start, \start, #1            // next entry
  add \phys, \phys, #SECTION_SIZE   // next block
  cmp \start, \end
  b.ls 9999b
.endm


__create_page_tables:
  mov x29, x30      // save return address

  adrp x0, pg_dir
  mov x1, #PG_DIR_SIZE
  bl memzero

  adrp x0, pg_dir
  mov x1, #VA_START
  create_pgd_entry x0, x1, x2, x3

  create_pud_entry x0, x1, x2, x3

  /* Mapping kernel and init stack */
  mov x1, xzr                                       // start mapping from physical offset 0
  mov x2, #VA_START                                 // first virtual address
  ldr x3, =(VA_START + DEVICE_BASE - SECTION_SIZE)  // last virtual address
  create_block_map x0, x1, x2, x3, MMU_FLAGS, x4


  /* Mapping device memory */
  mov x1, #DEVICE_BASE                  // start mapping from device base address
  ldr x2, =(VA_START + DEVICE_BASE)                     // first virtual address
  ldr x3, =(VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE) // last virtual address
  create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4

  add x0, x0, #PAGE_SIZE
  mov x1, #PHYS_MEMORY_SIZE
  ldr x2, =(VA_START + PHYS_MEMORY_SIZE)
  ldr x3, =(VA_START + 0x40000000 - SECTION_SIZE) // 1GB
  create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4

  mov x30, x29                                      // restore return address
  ret
