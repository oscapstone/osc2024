#include "mmu.h"

.section ".text.boot"

.global _start

_start:
    // Switch from EL2 to EL1 .
    bl from_el2_to_el1

set_mmu_configuration:
    // set paging configuration (up : 0xffff000000000000 low : 0x0000000000000000)
    ldr x4, = TCR_CONFIG_DEFAULT
    msr tcr_el1, x4

    // Set Used Memory Attributes
    ldr x4, =((MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)))
    msr mair_el1, x4

    // set and enable MMU
    ldr x4, = MMU_PGD_ADDR // PGD's page frame at 0x1000
    ldr x1, = MMU_PUD_ADDR // PUD's page frame at 0x2000

    ldr x2, = BOOT_PGD_ATTR
    orr x2, x1, x2         // combine the physical address of next level page with attribute.
    str x2, [x4]           // [MMU_PGD_ADDR] = MMU_PUD_ADDR | BOOT_PGD_ATTR

    ldr x2, = BOOT_PUD_ATTR
    mov x3, 0x00000000
    orr x3, x2, x3         // [MMU_PUD_ADDR] = 0x00000000 | BOOT_PUD_ATTR
    str x3, [x1]           // 1st 1GB mapped by the 1st entry of PUD
    mov x3, 0x40000000
    orr x3, x2, x3         // [MMU_PUD_ADDR + 8] = 0x40000000 | BOOT_PUD_ATTR
    str x3, [x1, 8]        // 2nd 1GB mapped by the 2nd entry of PUD

    msr ttbr0_el1, x4      // load PGD to the bottom translation-based register.
    msr ttbr1_el1, x4      // also load PGD to the upper translation based register.

    mov sp, 0x3c000000     // temp stack
    bl set_2M_kernel_mmu

    mrs x2, sctlr_el1      // sctlr_el1: Provides top level control of the system, including its memory system, at EL1 and EL0.
    orr x2 , x2, 1         // sctlr_el1[0]: EL1&0 stage 1 address translation enabled/disabled.
    msr sctlr_el1, x2

    // indirect branch to the upper virtual address
    ldr x2, =set_exception_vector_table
    br x2

set_exception_vector_table:
    adr x1, exception_vector_table // vbar_el1: Vector Base Address Register (EL1)
    msr vbar_el1, x1               //           Holds the exception base address for any exception that is taken to EL1.

setup_stack:
    // set top of stack at 0xffff00003c000000 (last usable memory)
    ldr x3, =_kernel_stack_top
    mov sp, x3

setup_bss:
    ldr     x1, =_bss_top
    ldr     w2, =_bss_size

init_bss:
    cbz     w2, run_main
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, init_bss

run_main:
    bl      main

proc_hang:
    wfe                            // waiting in low-power state
    b       proc_hang

from_el2_to_el1:
    mov x1, (1 << 31)              // hcr_el2: Execution state control for EL2
    msr hcr_el2, x1                //          RW[31]: 0b1 The processor execution environment for EL1 is AArch64
    mov x1, 0x3c5                  // spsr_el2: Holds the saved process state when an exception is taken to EL2.
    msr spsr_el2, x1               //           (1)[1111] 00 (2)[0101] -> (1) EL2-PSTATE.DAIF Disabled (2) Exception level = EL1h
    msr elr_el2, lr                // elr_el2: When taking an exception to EL2, holds the address to return to.
    eret                           // eret: Perform an exception return. EL2 -> EL1
