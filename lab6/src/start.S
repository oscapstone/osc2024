.section ".text.boot"

#include "vm.h"

.global _start
_start:
    mov x10, x0 // save dtb address

    /* switch from EL2 to EL1 */
    bl from_el2_to_el1

    ldr x0, =TCR_CONFIG_DEFAULT
    msr tcr_el1, x0

    ldr x0, =MAIR_CONFIG_DEFAULT
    msr mair_el1, x0

    mov x0, 0x1000
    mov x1, 0x2000

    ldr x2, =BOOT_PGD_ATTR
    orr x2, x1, x2
    str x2, [x0]

    ldr x2, =BOOT_PUD_ATTR
    mov x3, 0x00000000
    orr x3, x3, x2
    str x3, [x1]
    mov x3, 0x40000000
    orr x3, x3, x2
    str x3, [x1, 8]

    msr ttbr0_el1, x0
    msr ttbr1_el1, x0
    mrs x2, sctlr_el1
    orr x2, x2, 1
    msr sctlr_el1, x2

    ldr x2, =boot_rest
    br x2

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
    adr x0, exception_vector_table // defined in excasm.S
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
