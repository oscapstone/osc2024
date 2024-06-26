.section ".text.boot"

.globl _start

_start:
    adr x1, .
    ldr x2, =_start
    ldr w3, =_bl_size

/* Copy our code to the link address */
1:  ldr x4, [x1], #8
    str x4, [x2], #8
    sub w3, w3, #1
    cbnz w3, 1b

/* Set the top of the stack be the start of our code */
    ldr x1, =_start
    mov sp, x1

/* Clear the BSS */
    ldr x1, =_bss_start
    ldr w2, =_bss_size
3:  cbz w2, 4f
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, 3b

/* jump to relocated c code */
4:  bl main-0x20000
    b 1b   // should not get here
