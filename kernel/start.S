.section .text.start
.global _start


#define CORE0_TIMER_IRQ_CTRL 0x40000040
#define EXPIRE_PERIOD 0xfffffff
_start:
    // move devicetree's addr (ENABLE WHEN USING QEMU)
    mov     x28, x0

    // switch from EL2 to EL1
    bl      from_el2_to_el1


    // set stack pointer
    ldr     x1, =__stack_top
    mov     sp, x1

    // clear bss segment
    ldr     x1, =__bss_start
    ldr     x2, =__bss_size
1:  cbz     x2, 2f
    str     xzr, [x1], #8
    sub     x2, x2, #1
    cbnz    x2, 1b

// TODO: move to init timer
core_timer_enable:
    mov x0, 1
    msr cntp_ctl_el0, x0 // enable

    mov x0, EXPIRE_PERIOD
    msr cntp_tval_el0, x0 // set expired time

    mov x0, 2
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str x0, [x1] // unmask timer interrupt

    // jump to C code
2:  bl      main

// 一開始在 EL2，so issue eret 時會從 EL2 系列的 system register 讀值
// 想像成收到 exception 進到 EL2，再發 eret 回到 EL1
from_el2_to_el1:
    mov     x0, (1 << 31)
    msr     hcr_el2, x0     // use AARCH64 (64-bit)
    mov     x0, 0x3c5       // EL1h (SPSel = 1) with interrupt disabled
    msr     spsr_el2, x0
    msr     elr_el2, lr
    eret                    // return to EL1
