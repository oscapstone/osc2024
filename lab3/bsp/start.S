.section ".data"
.global dtb_base
dtb_base: .word 0

.section ".text.boot"

.global _start

_start:
    // store dtb base address from x0 to dtb_base
    ldr x1, =dtb_base
    str x0, [x1]
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, boot
    // cpu id > 0, stop

proc_hang:  
    wfe
    b       proc_hang

boot:
    // set exception levels to EL1, where the kernel will run
    bl from_el2_to_el1             // from exception.S
    // set exception vector table
    bl set_exception_vector_table  // from exception.S

bss_init:  
    // cpu id == 0
    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

bss_clear:  
    cbz     w2, jump_main
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, bss_clear

    // jump to C code, should not return
jump_main:  
    bl      main
    // for failsafe, halt this core too
    b       proc_hang
