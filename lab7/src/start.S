.section ".text.boot"

#include "vm.h"

.global _start
_start:
    mov x10, x0 // save dtb address

    /* switch from EL2 to EL1 */
    bl from_el2_to_el1

    /* set up kernel page tables */
    ldr x0, =TCR_CONFIG_DEFAULT
    msr tcr_el1, x0

    ldr x0, =MAIR_CONFIG_DEFAULT
    msr mair_el1, x0

    mov x0, 0x1000 // PGD base addr
    mov x1, 0x2000 // PUD base addr
    mov x2, 0x3000 // PMD base addr

    ldr x6, =PD_TABLE
    orr x6, x1, x6
    str x6, [x0]

    ldr x6, =PD_TABLE
    orr x6, x2, x6
    str x6, [x1]

    ldr x6, =PD_DEVICE_ATTR
    mov x3, 0x40000000
    orr x6, x3, x6
    str x6, [x1, 8]

    /* set PMD entries */
    mov x3, 0x00000000
    mov x4, 0x3F000000
    mov x5, 0x40000000

set_pmd:
    cmp x3, x5
    beq enable_mmu
    cmp x3, x4
    bge set_attr_device

set_attr_normal:
    ldr x6, =PD_NORMAL_ATTR
    b set_entry

set_attr_device:
    ldr x6, =PD_DEVICE_ATTR
    b set_entry

set_entry:
    orr x6, x3, x6
    str x6, [x2], #8
    add x3, x3, 0x200000
    b set_pmd

enable_mmu:
    /* enable memory management unit */
    msr ttbr0_el1, x0
    msr ttbr1_el1, x0

    mrs x0, sctlr_el1
    orr x0, x0, 1
    msr sctlr_el1, x0

    ldr x0, =boot_rest
    br x0

boot_rest:
    /* get dtb address from x0 */
    mov x0, x10 // restore dtb address
    ldr x1, =DTB_BASE // defined in devtree.c
    str x0, [x1]

    /* get cpu id */
    mrs x1, mpidr_el1
    and x1, x1, #3
    cbnz x1, halt // halt if cpu id != 0

    /* set exception vector table */
    adr x0, exception_vector_table // defined in entry.S
    msr vbar_el1, x0

    /* set stack pointer */
    ldr x1, =_start
    mov sp, x1

    /* clear bss section */
    ldr x1, =__bss_start
    ldr w2, =__bss_size

bss_reset:
    cbz w2, run_main
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, bss_reset

run_main:
    /* branch to main function */
    bl main

halt:
    wfe
    b halt

from_el2_to_el1:
    mov x0, (1 << 31)
    msr hcr_el2, x0 // EL1 uses aarch64
    mov x0, 0x3C5
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1
