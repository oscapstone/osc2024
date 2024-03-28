# To keep this in the first protion of the binary.
.section ".text.boot"

.global _start

# https://developer.arm.com/documentation/ddi0500/j/System-Control/AArch64-register-descriptions/Multiprocessor-Affinity-Register
_start:
    mrs x0, mpidr_el1
    and x0, x0, #3
    cbz x0, set_sp

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