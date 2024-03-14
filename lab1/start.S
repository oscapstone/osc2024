.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0, stop
1:  wfe
    b       1b 
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