.global _dtb_ptr
.section .data
_dtb_ptr: .dc.a 0x0

.section ".text.boot"

.global _start

# https://developer.arm.com/documentation/ddi0500/j/System-Control/AArch64-register-descriptions/Multiprocessor-Affinity-Register
_start:
    ldr x1, =_dtb_ptr
    str x0, [x1]

    mrs x2, mpidr_el1
    and x2, x2, #3
    cbz x2, set_sp

halt:
    wfe
    b halt

set_sp:
    ldr x0, = _start
    mov sp, x0
    ldr x0, = __bss_start
    ldr x1, = __bss_size

initialize_bss:
    cbz x1, kernel_main
    str xzr, [x0], #8
    sub x1, x1, #1
    b initialize_bss

kernel_main:
    bl main
    b halt