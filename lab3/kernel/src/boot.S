/* ARMv8 Assembly */

.section ".text.boot"

.global _start

_start:
    bl from_el2_to_el1

set_exception_vector_table:
    adr x1, exception_vector_table // vbar_el1: Vector Base Address Register (EL1)
    msr vbar_el1, x1               //           Holds the exception base address for any exception that is taken to EL1.

setup_stack:
    ldr     x1, =_stack_top
    mov     sp, x1                 // sp = x1

setup_bss:
    ldr     x1, =_bss_top          // load 32bits to x1 from _bss_top
    ldr     w2, =_bss_size         // load 64bits to w2 from _bss_size

init_bss:
    cbz     w2, run_main
    str     xzr, [x1], #8          // store xzr to addr[x1], then x1=x1+8
    sub     w2, w2, #1
    cbnz    w2, init_bss           // branch if w2 != 0

run_main:
    ldr     x1, =dtb_ptr
    str     x0, [x1], #8
    bl      main                   // branch and update lr with "main"

proc_hang:
    wfe                            // waiting in low-power state
    b       proc_hang

from_el2_to_el1:
    mov x1, (1 << 31)
    msr hcr_el2, x1
    mov x1, 0x3c5
    msr spsr_el2, x1
    msr elr_el2, lr
    eret

// mov x1, (1 << 31)
// msr hcr_el2, x1
// hcr_el2: page 1923, 64-bit register. Configuration controls for EL2.
// hcr_el2[31]: RW bit, indicates EL1 runs in AArch64.

// mov x1, 0x3c5
// msr spsr_el2, x1
// spsr_el2: page 283, 32-bit register. Holds the saved process state when an exception is taken to EL2.
// 0011: bit [11:10]: reserved. D(bit [9]): mask bit of watchpoint, breakpoint, etc. A(bit [8]): SError mask bit.
// 1100: I(bit [7]): IRQ mask bit. F(bit [6]): FIQ mask bit. bit [5]: reserved. bit [4]: 0 means exception taken from AArch64.
// 0101: M[3:0](bit [3:0])Exception was taken from EL1h. h(M[0]) means the saved processor state is determined by the EL.

// msr elr_el2, lr
// elr_el2: page 259, 64-bit register. When taking an exception to EL2, holds the address to return to.
// lr: Link Register, holds the return address.

// eret
// Exception return from EL2 to EL1.