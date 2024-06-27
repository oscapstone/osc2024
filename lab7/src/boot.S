#include "mmu.h"

.section ".text.boot"

.global _start

_start:
    bl from_el2_to_el1

set_mmu_configuration:
    ldr x4, = TCR_CONFIG_DEFAULT
    msr tcr_el1, x4

    ldr x4, =( \
        (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
        (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) \
    )
    msr mair_el1, x4

    // set and enable MMU
    mov x4, 0x1000  // PGD's address
    mov x5, 0x2000  // PUD's address

    ldr x6, = BOOT_PGD_ATTR
    orr x6, x5, x6
    str x6, [x4]

    ldr x6, = BOOT_PUD_ATTR
    mov x7, 0x0
    orr x7, x6, x7
    str x7, [x5]
    mov x7, 0x40000000
    orr x7, x6, x7
    str x7, [x5, #8]

    msr ttbr0_el1, x4
    msr ttbr1_el1, x4

    mov sp, 0x3c000000
    bl set_2M_kernel_mmu

    mrs x6, sctlr_el1
    orr x6, x6, #1
    msr sctlr_el1, x6

    // indirect branch to the next instruction
    ldr x6, = boot_rest
    br x6

boot_rest:
    adr x1, exception_vector_table
    msr vbar_el1, x1

setup_stack:
    ldr x3, =_stack_top
    mov sp, x3

setup_bss:
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

init_bss:
    cbz     w2, kernel_main
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, init_bss

kernel_main:
    ldr     x1, =dtb_ptr
    str     x0, [x1], #8
    bl      main                   // branch and update lr with "main"

proc_hang:
    wfe                            // waiting in low-power state
    b       proc_hang

from_el2_to_el1:
    mov x20, (1 << 31)       // EL1 uses aarch64
    msr hcr_el2, x20         // move x0 to the HCR_EL2 register
    mov x20, 0x3c5           // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x20        // move x0 to the SPSR_EL2 register
    msr elr_el2, lr         // move the link register to the ELR_EL2 register
    eret                    // return to EL1