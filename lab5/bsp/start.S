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


set_exception_vector_table:
    adr x0, exception_vector_table  // get the base address of the exception vector table
    msr vbar_el1, x0                // set the base address of the exception vector table
    ret

from_el2_to_el1:
    mov x0, (1 << 31)   // EL1 uses aarch64
    msr hcr_el2, x0     // msr: Status Register <-- Register (page 1924)
    mov x0, 0x3c5       // EL1h (SPSel = 1) with interrupt disabled (D = 1, A = 1, I = 1, F = 1, M[3:0] = 5)
    msr spsr_el2, x0    // Save the current processor's state
    msr elr_el2, lr     // Save the exception return address
    eret                // return to EL1 (use eret to return from the exception)
