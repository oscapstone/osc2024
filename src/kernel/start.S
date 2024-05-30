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
    // setup virtual memory

    ldr     x0, =TCR_CONFIG_DEFAULT
    msr     TCR_EL1, x0

    ldr     x0, =( \
        (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
        (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) \
    )
    msr     MAIR_EL1, x0

    ldr     x0, =__phy_upper_PGD // PGD's page frame
    ldr     x1, =__phy_upper_PUD // PUD's page frame

    ldr     x2, =BOOT_PGD_ATTR
    orr     x2, x1, x2 // combine the physical address of next level page with attribute.
    str     x2, [x0]

    ldr     x2, =BOOT_PUD_ATTR
    mov     x3, #0x00000000
    orr     x3, x2, x3
    str     x3, [x1] // 1st 1GB mapped by the 1st entry of PUD
    mov     x3, #0x40000000
    orr     x3, x2, x3
    str     x3, [x1, 8] // 2nd 1GB mapped by the 2nd entry of PUD

    msr     TTBR0_EL1, x0 // load PGD to the bottom translation-based register.
    msr     TTBR1_EL1, x0 // also load PGD to the upper translation based register.

    mrs     x0, SCTLR_EL1
    // disable alignment fault checking
    bic     x0, x0, #2
    // enable MMU, cache remains disabled
    orr     x0 , x0, #1
    msr     SCTLR_EL1, x0

    ldr     x2, =boot_rest // indirect branch to the virtual address
    br      x2

boot_rest:
    ADR_REL x0, __stack_end
    mov     sp, x0

    mov     x0, xzr

    dsb     ISH             // ensure write has completed
    msr     TTBR0_EL1, x0
    tlbi    VMALLE1IS       // invalidate all TLB entries
    dsb     ISH             // ensure completion of TLB invalidatation
    isb                     // clear pipeline

    bl      set_exception_vector_table

    ADR_REL x0, __bss_start
    ADR_REL x1, __bss_end
    bl      memzero

    // restore dtb addr
    mov     x0, x19
    bl      kernel_main

    b       prog_hang
