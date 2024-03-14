.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    // mrs (Move Register to System register)
    // mpidr_el1是一個系統寄存器，全名為"Multiprocessor Affinity Register (EL1)"
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    // cbz (Compare and Branch on Zero)
    cbz     x1, 2f
    // cpu id > 0, stop
    // wfe (Wait For Event)
1:  wfe
    // b (Branch)
    b       1b
2:  // cpu id == 0

    // set stack before our code
    // ldr (Load Register)
    ldr     x1, =_start
    // mov (MOVe)
    // sp是堆疊指針（Stack Pointer）
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    // str (SToRe)
    str     xzr, [x1], #8
    // sub (SUBtract)
    sub     w2, w2, #1
    // cbnz (Compare and Branch on Non-Zero)
    cbnz    w2, 3b

    // jump to C code, should not return
    // bl (Branch with Link)
4:  bl      main
    // for failsafe, halt this core too
    b       1b