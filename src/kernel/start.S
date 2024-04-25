#include "arm.hpp"

// - https://sourceware.org/binutils/docs-2.36/as/AArch64_002dRelocations.html
.macro ADR_REL register, symbol
	adrp	\register, \symbol
	add	\register, \register, #:lo12:\symbol
.endm

.section ".text._start"

.global _start
_start:
    // save dtb addr
    mov     x19, x0

    mrs     x0, MPIDR_EL1
    and     x0, x0, #0x3
    cbz     x0, primary_cpu

.global prog_hang
prog_hang:
    wfe
    b       prog_hang

primary_cpu:
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    cmp     x0, #1
    beq     primary_cpu_el1

    mov     x0, HCR_VALUE
    msr     HCR_EL2, x0
    mov     x0, SPSR_VALUE
    msr     SPSR_EL2, x0
    ADR_REL x0, primary_cpu_el1
    msr     ELR_EL2, x0
    eret

primary_cpu_el1:
    ADR_REL x0, __stack_end
    mov     sp, x0

    bl      set_exception_vector_table

    ADR_REL x0, __bss_start
    ADR_REL x1, __bss_end
    bl      memzero

    // restore dtb addr
    mov     x0, x19
    bl      kernel_main

    b       prog_hang
