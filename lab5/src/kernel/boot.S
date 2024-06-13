.section ".text"

.global _start

_start:
    mrs x1, mpidr_el1       // read the MPIDR_EL1 register; mrs = move from system register
    and x1, x1, #0x3
    cbz x1, cpu0            // cbz = compare and branch if zero; if x0 is zero, branch to cpu0

halt:
    wfe                     // wait for event
    b halt

cpu0:
    mov x21, x0
    bl from_el2_to_el1
    bl set_exception_vector_table

    ldr x1, =_start         // load the address of _start into x0
    mov sp, x1              // set the stack pointer to the address of _start
    ldr x1, =__bss_start    // load the address of __bss_start into x0
    ldr w2, =__bss_size

clear_bss:
    cbz w2, kernel_main
    str xzr, [x1], #8       // store zero to the address in x0, then increment x0 by 8
    sub w2, w2, #1
    cbnz w2, clear_bss

kernel_main:
    bl main                 // jump to C code
    b halt

from_el2_to_el1:
    mov x20, (1 << 31)       // EL1 uses aarch64
    msr hcr_el2, x20         // move x0 to the HCR_EL2 register
    mov x20, 0x3c5           // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x20        // move x0 to the SPSR_EL2 register
    msr elr_el2, lr         // move the link register to the ELR_EL2 register
    eret                    // return to EL1


set_exception_vector_table:
    adr x0, exception_vector_table
    msr vbar_el1, x0
    ret
