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

    ldr x3, = _start
    mov sp, x3          // set sp to correct position (grow down)

    ldr x0, bss_start
    ldr x1, bss_end
mem_zero:
// zero out memory x0(low) - x1(high)
    str xzr, [x0], #8  // xzr is zero register
    cmp x0, x1          // compare if equal
    ble mem_zero


//    mov x3, #0xfe // testing gdb
    bl kernel_start
// not supposed to be here
    b proc_hang

// low power standby infinite loop
proc_hang:
    wfe
    b proc_hang
