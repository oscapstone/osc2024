.section ".text._start_section"

.global _start

.equ TCR_CONFIG_REGION_48bit, (((64 - 48) << 0) | ((64 - 48) << 16));
.equ TCR_CONFIG_4KB, ((0x0 << 14) |  (0x2 << 30));
.equ TCR_CONFIG_DEFAULT, (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB);

.equ MAIR_DEVICE_nGnRnE, 0b00000000
.equ MAIR_NORMAL_NOCACHE, 0b01000100
.equ MAIR_IDX_DEVICE_nGnRnE, 0
.equ MAIR_IDX_NORMAL_NOCACHE, 1
.equ MAIR_CONFIG_PART1, (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8))
.equ MAIR_CONFIG_PART2, (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8))
.equ MAIR_CONFIG_DEFAULT, (MAIR_CONFIG_PART1 | MAIR_CONFIG_PART2)

.equ PD_TABLE, 0b11
.equ PD_BLOCK, 0b01
.equ PD_ACCESS, (1 << 10)
.equ BOOT_PGD_ATTR, PD_TABLE
.equ BOOT_PUD_ATTR, (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)

_start:
    mrs x0, CurrentEL
    and x0, x0, #0xc
    cmp x0, #0b1000
    bne _loop

    bl from_el2_to_el1
    bl set_mmu

    // set stack before our code
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, _start_kernel
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b
_start_kernel:
    adr x0, exception_vector_table
    msr vbar_el1, x0

    ldr x0, =_start_rust
    br x0

from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    mov x0, sp
    msr sp_el1, x0
    eret // return to EL1

set_mmu:
    ldr x0, =TCR_CONFIG_DEFAULT
    msr tcr_el1, x0

    ldr x0, =MAIR_CONFIG_DEFAULT
    msr mair_el1, x0

    mov x0, 0x1000 // PGD's page frame at 0x0
    mov x1, 0x2000 // PUD's page frame at 0x1000

    ldr x2, =BOOT_PGD_ATTR
    orr x2, x1, x2 // combine the physical address of next level page with attribute.
    str x2, [x0]

    ldr x2, =BOOT_PUD_ATTR
    mov x3, 0x00000000
    orr x3, x2, x3
    str x3, [x1] // 1st 1GB mapped by the 1st entry of PUD
    mov x3, 0x40000000
    orr x3, x2, x3
    str x3, [x1, 8] // 2nd 1GB mapped by the 2nd entry of PUD

    msr ttbr0_el1, x0 // load PGD to the bottom translation-based register.
    msr ttbr1_el1, x0 // load PGD to the bottom translation-based register.
    isb

    mrs x2, sctlr_el1
    orr x2 , x2, 1
    msr sctlr_el1, x2 // enable MMU, cache remains disabled
    isb
    ret

_loop:
    b _loop
