.section ".text.boot"

.global _start

_start:
    // dtb in x12
    ldr     x13, =_dtb_ptr //put _dtb_ptr into register1
    str     x12, [x13] //store dtb address from x0 to _dtb_ptr
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1 // 读取 MPIDR_EL1 寄存器的值到寄存器 x1
    and     x1, x1, #3 // 将 x1 寄存器的值和 3 进行按位与操作
    cbz     x1, 2f // 如果 x1 寄存器的值为 0，则跳转到标签 2
    // cpu id > 0, stop
1:  wfe // 等待事件。CPU 进入低功耗状态，直到有一个事件发生，跳出loop
    b       1b // 无条件分支到标签 1
2:  // cpu id == 0

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start
    mov     sp, x1 // register x1 to rgister sp

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f //如果size=0, 就可以進main
    str     xzr, [x1], #8 //把0寫進x1這個記憶體位置後，x1 register的值就會加8個byte
    sub     w2, w2, #1 //bss的size就再減1
    cbnz    w2, 3b //如果w2的值!=0,則跳到3繼續跑

    // jump to C code, should not return
4:  bl      main //bl會存pc+1(因為要return)
    // for failsafe, halt this core too
    b       1b //b不會存pc+1

.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0