.section ".text.boot"

.global _start

_start:
    mrs x0, mpidr_el1       // read the MPIDR_EL1 register; mrs = move from system register
    and x0, x0, #0x3
    cbz x0, cpu0            // cbz = compare and branch if zero; if x0 is zero, branch to cpu0

halt:
    wfe                     // wait for event
    b halt

cpu0:
    ldr x0, =_start         // load the address of _start into x0
    mov sp, x0              // set the stack pointer to the address of _start
    ldr x0, =__bss_start    // load the address of __bss_start into x0
    ldr w2, =__bss_size

clear_bss:
    cbz w2, kernel_main
    str xzr, [x0], #8       // store zero to the address in x0, then increment x0 by 8
    sub w2, w2, #1
    cbnz w2, clear_bss

kernel_main:
    bl main                 // jump to C code
    b halt