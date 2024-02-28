// <name> equ <value> // define constant

.section ".text.boot"

.global _start

_start:
    eor x0, x0, x0
    mov sp, x0          // clear sp

    bl main
    b proc_hang         // not supposed to be here

// low power standby infinite loop
proc_hang:
    wfe
    b proc_hang
