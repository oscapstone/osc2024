    .section ".text.boot"
    .global _start
_start:
    mov     x10, x0

    // read cpu id, hang slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #0xFF       // Check processor id in last 8 bits
    cbnz    x1, _hang          

_master:
    // switch exception level from EL2 to EL1
    bl      _el2_to_el1

    // set vbar_el1 register
    bl      set_exception_vector_table   

    // set top of stack just before our code (0x80000)
    ldr     x1, = kernel_start
    mov     sp, x1

    // clear bss
    bl      _memzero        

    // jump to c, should not return
    mov     x0, x10
    bl      kernel_main

    // Hang for all non-primary CPU and this core when c code failed
_hang:
    wfe
    b       _hang

_memzero:
    ldr    x0, =_bss_begin
    ldr    x1, =_bss_size
_setzero:    
    str     xzr, [x0], #8
    subs    x1, x1, #8
    b.gt    _setzero
    ret


_el2_to_el1:
    mov     x1, (1 << 31)           // hcr_el2: Execution state control for EL2
    msr     hcr_el2, x1             // [31]: 1 = EL1 is AArch64, 0 = EL1/EL0 is AArch32
    
    mov     x1, 0x3c5               // spsr_el2: Holds the saved process state when an exception is taken to EL2.
    msr     spsr_el2, x1            // [1111]00[0101] -> (1) EL2-PSTATE.DAIF Disabled (2) Exception level = EL1h
    
    msr     elr_el2, lr             // elr_el2: When taking an exception to EL2, holds the address to return to.
    eret                            // Perform an exception return. EL2 -> EL1


/*

SPSR_EL2:

D,bit[9]:   Process state Debug mask
A,bit[8]:   SError (System Error) mask bit
I,bit[7]:   IRQ mask bit
F,bit[6]:   FIQ mask bit
M[4]:       Execution state that the exception was taken from. A value of 0 indicates AArch64.
M[3:0]:     AArch64 Exception level and selected Stack Pointer.


M[3:0]      Meaning
------------------------------------
0b0000	    EL0.
0b0100	    EL1 with SP_EL0 (ELt).
0b0101	    EL1 with SP_EL1 (EL1h).
0b1000	    EL2 with SP_EL0 (EL2t).
0b1001	    EL2 with SP_EL2 (EL2h).

 */


    .global current_exception_level
current_exception_level:
    mrs     x0, CurrentEl
    lsr     x0, x0, #2          // remove reserved bits
    and     x0, x0, 0x3
    ret


    .global align
align:
    sub     x1, x1, #1
    add     x0, x0, x1
    mvn     x1, x1
    and     x0, x0, x1
    ret
