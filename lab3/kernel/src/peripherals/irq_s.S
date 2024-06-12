
// ARMv8 pg. 254 DAIF, mask and unmask the irq bit
.global irq_enable
irq_enable:
    msr     daifclr, #2
    ret

.global irq_disable
irq_disable:
    msr     daifset, #2
    ret