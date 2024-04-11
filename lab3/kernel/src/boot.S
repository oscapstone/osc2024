/* ARMv8 Assembly */

.section ".text.boot"

.global _start

_start:
    bl from_el2_to_el1             // 下一條指令存到LR後，跳到from_el2_to_el1函數

set_exception_vector_table:        // 設定exception table 的位址到 vbar_el1 
    adr x1, exception_vector_table // vbar_el1: Vector Base Address Register (EL1)
    msr vbar_el1, x1               // Holds the exception base address for any exception that is taken to EL1.

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

from_el2_to_el1:                   //從EL2安全地返回到EL1
    mov x1, (1 << 31)              // 將x1暫存器設置為1+後面31個0，為了配置hcr_el2: 這個暫存器控制EL2的狀態 
    msr hcr_el2, x1                // x1 write to hcr_el2
                                   // 把RW bit設為1代表EL1執行環境為AArch64
    mov x1, 0x3c5                  // 0x3c5的值寫入x1
                                   // spsr_el2: 保存狀態，當從EL2返回時，處理器應該返回的狀態
    msr spsr_el2, x1               // (1)[1111] 00 (2)[0101] -> (1) EL2-PSTATE.DAIF Disabled (2) Exception level = EL1h 使用SPx stack pointer
                                   // set spsr_el2 = 0b1111000101
    msr elr_el2, lr                // elr_el2: 從EL2返回到原本的程序，下一條要執行指令的位址；lr: Link reg 儲存return addr 
    eret                           // eret: EL2 返回至 EL1; 按照spsr_el2+elr_el2恢復狀態
