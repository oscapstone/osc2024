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

    mov x3, #0xde // testing gdb
    mov sp, x3          // clear sp

clear_bss:
// zero out the bss section
    ldr x0, bss_start
    ldr x1, bss_end


    mov x3, #0xfe // testing gdb
    bl kernel_start
// not supposed to be here
    b proc_hang

// low power standby infinite loop
proc_hang:
    wfe
    b proc_hang
