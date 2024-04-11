/* ARMv8 Assembly */

.section ".text.boot"

.global _start

_start:
    bl from_el2_to_el1

set_exception_vector_table:
    adr x1, exception_vector_table // 將例外向量表的位址加載到x1 // exception_vector_table在entry.S,檔案中 // vbar_el1: Vector Base Address Register (EL1) 
    msr vbar_el1, x1               // 將x1中的位址指定為EL1狀態下的例外處理的base address
                                   // 這條指令就是在告訴處理器，當意料之外的事發生在EL1這個安全層級時，
                                   // 它可以從x1暫存器中記下的地址開始找到處理這些意外情況所需的所有信息和指令。
                                   // Holds the exception base address for any exception that is taken to EL1.

setup_stack:
    ldr     x1, =_stack_top        // 將stack top的位址加載到x1
    mov     sp, x1                 // 將x1的值（stack top的位址）設置給stack指針（sp）// sp = x1

setup_bss:
    ldr     x1, =_bss_top          // 將BSS段頂端的位址加載到暫存器x1中 // load 32bits to x1 from _bss_top
    ldr     w2, =_bss_size         // 將BSS段的大小加載到暫存器w2 // load 64bits to w2 from _bss_size

init_bss:
    cbz     w2, run_main
    str     xzr, [x1], #8          // store xzr to addr[x1], then x1=x1+8
    sub     w2, w2, #1
    cbnz    w2, init_bss           // 如果寄存器w2為零（即BSS段的大小為零），則跳轉到 run_main 標籤 // branch if w2 != 0

run_main:
    ldr     x1, =dtb_ptr           // 將設備樹（Device Tree Blob）的位址加載到暫存器x1中
    str     x0, [x1], #8           // 將x0（通常包含了從bootloader傳來的一個值，如設備樹的位址）存到x1指向的位址，並將x1增加8。
    bl      main                   // branch and update lr with "main"

proc_hang:
    wfe                            // waiting in low-power state
    b       proc_hang

from_el2_to_el1:                   // Basic Exercise 1 - Exception
    mov x1, (1 << 31)              // 將x1設置為2的31次方(即將1位左移31位)，這樣x1的值將只有最高位為1，其餘位為0 
    msr hcr_el2, x1                // 將暫存器x1的值(只有最高位為1的32bits)寫入到HCR_EL2暫存器中，設置異常層級2的控制狀態。這裡這樣設置會使EL1執行環境為AArch64的模式 
                                   // hcr_el2: Execution state control for EL2
                                   //          RW[31]: 0b1 The processor execution environment for EL1 is AArch64
    mov x1, 0x3c5                  // 將數值0x3c5（二進制的1111 00 0101）移入x1中：1111代表的是Disabled El2-PSTATE.DAIF/0101代表的是Exception level = EL1h
                                   // 這個值配置了處理器狀態，包括(1111)禁用了中斷和(0101)設置了異常層級為EL1
    msr spsr_el2, x1               // 這個spsr_el2暫存器保存當EL2發生異常時的處理器狀態
                                   // spsr_el2: Holds the saved process state when an exception is taken to EL2.
                                   //           (1)[1111] 00 (2)[0101] -> (1) EL2-PSTATE.DAIF Disabled (2) Exception level = EL1h
    msr elr_el2, lr                // 將鏈接暫存器(lr)的值寫入到ELR_EL2，這是當為了當發生異常時轉入EL2後保存返回位址
                                   // elr_el2: When taking an exception to EL2, holds the address to return to.
    eret                           // eret: Perform an exception return. EL2 -> EL1
