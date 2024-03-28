/* ARMv8 Assembly Instruction */
/**

mov x0, x1
    sets: x0 = x1
ldr x0, <addr>
    load 32bits from <addr> to x0
ldr w0, <addr>
    load 64bits from <addr> to w0
cbz x0, <label>
    if x0 == 0, jump to <label>
cbnz x0, <label>
    if x0 != 0, jump to <label>
str x0 [x1] #8
    store x0 in addr<x1> then x1=x1+8
b   <label>
    jump to <label>
bl  <label>
    jump to <label> and copies bl's next instruction into link register
wfe
    Wait for event, core in low-power state (power on, clk off)

**/

// x0 用於存儲設備樹（Device Tree Blob，DTB）的物理地址
.section ".text.boot"  // 指定節（section），.text.boot 通常用於啟動代碼

.global _start  // 聲明全局入口點 _start

_start:  // 程序入口點
setup_stack:  // 設置堆棧
    ldr     x1, =_stack_top  // 加載堆棧頂部地址到 x1
    mov     sp, x1  // 將 x1 的值（堆棧頂部地址）移動到堆棧指針 sp

setup_bss:  // 設置 BSS 段（未初始化數據）
    ldr     x1, =_bss_top  // 加載 BSS 段的起始地址到 x1
    ldr     w2, =_bss_size  // 加載 BSS 段的大小到 w2

init_bss:  // 初始化 BSS 段，將其清零
    cbz     w2, run_main  // 如果 w2（BSS 大小）為零，則跳轉到 run_main
    str     xzr, [x1], #8  // 將寄存器 xzr（值為 0 的寄存器）的內容存儲到 x1 指向的地址，然後 x1 加 8
    sub     w2, w2, #1  // w2 減 1
    cbnz    w2, init_bss  // 如果 w2 不為零，則跳回 init_bss 繼續初始化

run_main:  // 運行主程序
    bl      main  // 跳轉到 main 函數，並保存返回地址

proc_hang:  // 進程掛起，等待事件
    wfe  // 等待事件
    b       proc_hang  // 無限循環，保持掛起狀態

