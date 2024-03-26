/* ARMv8 Assembly Instruction */
/**

mov x0, x1
    sets: x0 = x1
ldr x0, <addr>
    load 32bits from <addr> to x0
ldr w0, <addr>
    load 32bits from <addr> to w0
cbz x0, <label> // compare and branch, zero
    if x0 == 0, jump to <label>
cbnz x0, <label> // compare and branch, non-zero
    if x0 != 0, jump to <label>
str x0 [x1] #8 
    store x0 in addr<x1> then x1=x1+8
    // #8 是因為地址為 8 bytes
b   <label> // 無條件jump to <label>
    jump to <label>
bl  <label> // jump to <label> 
    // 並將下一個指令的地址（return addr）存在link暫存器中
    jump to <label> and copies bl's next instruction into link register
wfe // 等待事件，使核心進入低功耗狀態（電源開啟，時鐘關閉）。
    Wait for event, core in low-power state (power on, clk off)

**/

// x0 is used for dtb physical address
.section ".text.boot"

.global _start

_start:
// 初始化堆疊 (setup_stack)
setup_stack:
    ldr     x1, =_stack_top
    mov     sp, x1  // 將x1的value移到sp中，sp為stack pointer
// 初始化 BSS 區域 (setup_bss 和 init_bss)
setup_bss:
    ldr     x1, =_bss_top
    ldr     w2, =_bss_size
// 
init_bss:
    cbz     w2, run_main
    str     xzr, [x1], #8 // 8 bytes : x1=x1+8
    sub     w2, w2, #1    // 1 word : 剩餘的字數減 1 word
    cbnz    w2, init_bss  // 如果剩餘字數 w2 不為 0，跳轉回 init_bss 標籤繼續循環

run_main:
    ldr     x1, =_dtb   // 將設備樹 (Device Tree Blob, DTB) 的地址載入到 x1 暫存器中
    str     x0, [x1], #8
    bl      main        // "Branch with Link"：跳到main後，將下一個指令的return addr存進Link register中

proc_hang:
    wfe                 // Wait for event 
    b       proc_hang
