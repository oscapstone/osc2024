.section ".text.boot"

.global _start

_start:
    // mov dtb from x0 to x20
    mov x12, x0 // Move DTB from register x0 to x12
    // relocate the bootloader
    ldr x10, =0x80000 // Load the address 0x80000 into register x10
    ldr x11, =__bootloader_start // Load the address of __bootloader_start (0x60000) into register x11
    ldr w3, =__bootloader_size // Load the size of the bootloader (__bootloader_size) into register w3

relocate:
    ldr x4, [x10], #8 // 0x80000 存到x4, 移8個byte (因為一個register 64個bits)
    str x4, [x11], #8 // store the value to 0x60000
    sub w3, w3, #1
    cbnz w3, relocate

setting: // set the stack point 
    ldr x1, =_start
    mov sp, x1
    ldr x1, =__bss_start
    ldr w2, =__bss_size

clear_bss: // clear bss for bootloader.
    cbz w2, bootloader_main
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, clear_bss

bootloader_main:
    bl main - 0x20000 
    b bootloader_main