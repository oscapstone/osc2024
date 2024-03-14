// .equ <name>, <value> // define constant
.equ LOWER_2, 0x03

.section ".text.boot"

.global _start

_start:
// only one cpu up, others go to sleep
    mrs x1, MPIDR_EL1
    and x1, x1, LOWER_2
    cbnz x1, proc_hang
// jump to hang if not core #0

    ldr x0, = __bss_start
    ldr x1, = __bss_end
    sub x1, x1, x0
    bl mem_zero

// set sp to correct position (grow down)
    ldr x1, = _start
    mov sp, x1

    bl kernel_start
// not supposed to be here
    b proc_hang

// low power standby infinite loop
proc_hang:
    wfe
    b proc_hang
