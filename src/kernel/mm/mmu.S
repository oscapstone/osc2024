.macro LOAD_TLB r1
    dsb     ISH             // ensure write has completed
    msr     TTBR0_EL1, \r1  // switch translation based address.
    tlbi    VMALLE1IS       // invalidate all TLB entries
    dsb     ISH             // ensure completion of TLB invalidatation
    isb                     // clear pipeline
.endm

.global load_tlb
load_tlb:
    LOAD_TLB    x0
    ret

.global reload_tlb
reload_tlb:
    dsb     ISH             // ensure write has completed
    tlbi    VMALLE1IS       // invalidate all TLB entries
    dsb     ISH             // ensure completion of TLB invalidatation
    isb                     // clear pipeline
    ret
