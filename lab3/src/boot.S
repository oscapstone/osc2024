#include "mm.h"
#include "arm/sysregs.h"
#include "exception.h"

.section ".text.boot"
.globl _start
_start:

    mov    x3, x0
    mrs    x0, mpidr_el1
    and    x0, x0, #0xFF
    cbz    x0, master
    b    proc_hang
proc_hang:
    wfe
    b proc_hang


master:
    bl     el2_to_el1
    adr    x0, bss_begin
    adr    x1, bss_end
    sub    x1, x1, x0
    bl     memzero
    mov    sp, #LOW_MEMORY
    // x3 stores devicetree addr
    mov    x0, x3
    bl     kernel_main