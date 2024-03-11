.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x0, mpidr_el1
    and     x0, x0, #3  //Check processor id
    cbz     x0, 2f      //if CPU id = 0, jump to 2f
    b       1f

1:  wfe
    b       1b

2:  ldr     x0, =_start
    mov     sp, x0
    ldr     x0, =__bss_start
    ldr     w1, =__bss_size

3:  cbz     w1, 4f
    str     xzr, [x0], #8  // x0* = xzr, x0 += 8 (write xzr represents discard, read xzr == 0)
    sub     w1, w1, #1
    cbnz    w1, 3b

4:  bl      main
    b       1b

// _start:
//  系統暫存器 mpidr_el1(cpu id) 載入到 通用暫存器 x0
//  x0 &= 011 (抓取末兩位數就可以拿到cpu id)
//  if(x0 == 0): forward 2
//  forward 1

// 1:
//  backward 1

// 2:
//  x0 = (_start)*
//  sp = x0  // 保存線頂地址
//  x0 = (__bss_start)*
//  w1 = (__bss_size)*

// 3:
//  if(w1 == 0): forward 4
//  *x0 = xzr; x0 += 8
//  w1 = w1 - 1
//  if(w1 != 0): backward 3

// 4:
//  to main.c
//  backward 1