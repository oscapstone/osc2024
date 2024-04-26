
// elutil_from_el1_to_el0(program address);
.global elutil_from_el1_to_el0
elutil_from_el1_to_el0:
    sub     sp, sp, #0x10
    str     x0, [sp, #8]
    msr     elr_el1, x0
    mov     x0, 0x3c0
    msr     spsr_el1, x0
    mov     x0, #0x60000
    msr     sp_el0, x0
    ldr     x0, [sp, #8]
    add     sp, sp, #0x10
    eret
