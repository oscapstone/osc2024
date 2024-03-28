#include "mm.h"

// specify boot.S should be in `.text.boot` section
.section ".text.boot"

.globl _start
_start:
    // _start get processor's id, only let one core run, while other cores stay idle
    // mpidr_el1 is register determined the processor ID
    mrs    x0, mpidr_el1
    // Check processor id       
    and    x0, x0, #0xFF
    // cbz branch if zero
    cbz    x0, master
    // Hang for all non-primary CPU
    b    proc_hang

proc_hang:
    // infinite idle loop
    // wfe, wait for event, low-power state, increase energy efficiency
    wfe
    b proc_hang

master: 
    // clean bss section
    adr    x0, bss_begin
    adr    x1, bss_end
    sub    x1, x1, x0
    // memzero, receive x0,x1, set zero from x0 to x0+x1
    // bl branch and store return address in x30, ret read x30 and return
    bl     memzero

    // initialize stack pointer, should be set any location high enough
    // #LOW_MEMORY is defined in "mm.h"
    mov    sp, #LOW_MEMORY
    
    bl    kernel_main