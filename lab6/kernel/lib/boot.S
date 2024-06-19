#include "mmu.h"

.section ".text.boot"

.global _start

_start:
  // read cpu id, stop slave cores
  // system register need use mrs
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    // switch to el1 before master
    cbz     x1, from_el2_to_el1
  // cpu id > 0, stop

busyloop:  
    wfe
    b       busyloop

from_el2_to_el1:
    bl _from_el2_to_el1


setup_kernel_vm:
    // set paging configuration (up : 0xffffxxxxxxxxxxxx low : 0x0000xxxxxxxxxxxx)
    ldr     x4, = TCR_CONFIG_DEFAULT
    
    msr     tcr_el1, x4

    // set Used Memory Attributes
    ldr     x4, =((MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)))
    msr     mair_el1, x4

    // set and enable MMU
    mov     x4, 0x1000 // PGD's page frame at 0x1000
    mov     x1, 0x2000 // PUD's page frame at 0x2000

    // PGD 12 ~ 2 ignore
    // set up PGD_attribute, pd table , 0b11
    ldr     x2, = BOOT_PGD_ATTR
    // physical  | address | attribute |
    orr     x2, x1, x2 // combine the physical address of next level page with attribute.
    // make first entry in PGD points to PUD
    str     x2, [x4]

    // set stack pointer location
    mov     sp, 0x3c000000 // startup stack
    bl      set_2M_kernel_mmu

    // load PGD to the bottom (user-space) translation-based register.
    msr     ttbr0_el1, x4 
    // load PGD to the upper  (kernel-space) translation based register.
    msr     ttbr1_el1, x4 

    // MMU enable for EL1 & 0 stage 1 address translation
    // cache remains disable
    // bits 0 : M
    mrs     x2, sctlr_el1
    orr     x2 , x2, 1
    msr     sctlr_el1, x2 

    ldr     x2, =master
    br      x2

master:
    ldr     x1, =(3 << 20)           // close advanced SIMD exception
    msr     CPACR_EL1, x1           // printf builtin_va might use
    adr     x1, exception_vector_table // set exception_vector_table
    msr     vbar_el1, x1

    // set top of stack at 0xffff00003c000000 (last usable memory)
    movz    x1, 0x0000
    movk    x1, 0x3c00, lsl 16
    movk    x1, 0xffff, lsl 48
    mov     sp, x1

    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

bssinitloop:  
    cbz     w2, kernel
    // save 0 and plus 8
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, bssinitloop

kernel:
    // bl save next command to lr register
    bl      main
    b       busyloop

_from_el2_to_el1:
    // enable RW
    mov x1, (1 << 31) // EL1 uses aarch64
    // decide execution state of el1 and el0
    msr hcr_el2, x1
    // 3c daif disabled
    // 5 exception level
    mov x1, 0x3c5 
    // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x1
    msr elr_el2, lr     // eret location
    eret // return to EL1 ,str in elr_el2