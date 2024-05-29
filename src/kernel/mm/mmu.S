.macro LOAD_PGD r1
    dsb     ISH             // ensure write has completed
    msr     TTBR0_EL1, \r1  // switch translation based address.
    tlbi    VMALLE1IS       // invalidate all TLB entries
    dsb     ISH             // ensure completion of TLB invalidatation
    isb                     // clear pipeline
.endm

.global load_pgd
load_pgd:
    LOAD_PGD    x0
    ret

.global reload_pgd
reload_pgd:
    dsb     ISH             // ensure write has completed
    tlbi    VMALLE1IS       // invalidate all TLB entries
    dsb     ISH             // ensure completion of TLB invalidatation
    isb                     // clear pipeline
    ret
