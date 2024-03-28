/*这段代码主要是在ARM架构的处理器上执行引导加载器的初始化，设置栈、清零BSS段、设置设备树地址，并最终跳转到主函数 main 执行。
*/

/* ARMv8 Assembly Instruction */
/**

mov x0, x1
    sets: x0 = x1
ldr x0, <addr>
    load 32bits from <addr> to x0
ldr w0, <addr>
    load 32bits from <addr> to w0
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
    wfe 存到一個暫存器裡面 
    因為bl做完要跳回來做wfe
wfe
    Wait for event, core in low-power state (power on, clk off)

**/

// x0 is used for dtb physical address
.section ".text.boot"

.global _start

_start:
setup_stack:
    ldr     x1, =_stack_top //加載stack_top到x1
    mov     sp, x1 //x1值移動到stack pointer

setup_bss:
    ldr     x1, =_bss_top
    ldr     w2, =_bss_size

init_bss:
    cbz     w2, run_main //若w2為0則跳到run main
    str     xzr, [x1], #8 
            //若w2還沒清完 就繼續往下清 往下一次是跳8bytes
    sub     w2, w2, #1 //bss size -1 words
    cbnz    w2, init_bss //size還不為0 繼續loop

run_main:
    ldr     x1, =_dtb
            //load DTB address to x1, DTB可以系統了解和管理硬體資源
    str     x0, [x1], #8
            //將x0的值存到x1指向的位址，x1+8 往下面跑 
            //初始化設備樹的地址
    bl      main
            //branch with link 跳轉回main並且將return address存到link register

proc_hang:
    wfe //Wait for event
    b       proc_hang
