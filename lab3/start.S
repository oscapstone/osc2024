.section ".text.boot"

.global _start

_start:
    /* get dtb address from x0 */
    ldr x1, =DTB_BASE // defined in devtree.c
    str x0, [x1]

    /* get cpu id */
    mrs x1, mpidr_el1
    and x1, x1, #3
    cbnz x1, halt // halt if cpu id != 0

    /* set exception vector table */
    adr x0, exception_vector_table // defined in excasm.S
    msr vbar_el1, x0

    /* switch from EL2 to EL1 */
    bl from_el2_to_el1

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
    mov x0, 0x345
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1
