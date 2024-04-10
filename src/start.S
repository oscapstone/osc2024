.section ".text.boot"

.global _start 

_start:
    mrs     x1, MPIDR_EL1   // get cpu id and put to reg x1
    and     x1, x1, #0x3      // Keep the lowest two bits
    cbz     x1, setting     // if cpu_id > 0, stop
proc_hang:
    wfe                     // wait-for-event instruction, which makes processors 
    b       proc_hang
setting:                      // if cpu_id == 0
    //mov     x27, x0
    bl      set_exception_vector_table
    ldr     x1, =_start       // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    mov     sp, x1
    ldr     x1, =__bss_start    // clear bss
    ldr     x2, =__bss_size
clear_bss:  
    cbz     x2, master     // if val in reg x2 is zero, thn jump to label 4, indicating that the .bss section has been zeroed.
    str     xzr, [x1], #8   // write a zero value (from the xzr register) to the memory address pointed to by x1, and then increment the value of x1, effectively zeroing the next 8 bytes.
    sub     x2, x2, #1
    cbnz    x2, clear_bss

setup_el1:
    bl      from_el2_to_el1
    //mov		sp, #0x40000		//put EL1 stack start from 0x40000	

master:  
    mov     x0, x27
    bl      main            // main function
    b       proc_hang       // halt this core if return

from_el2_to_el1:
    mov     x0, (1 << 31)   // RW, bit[31] (in HCR_EL2 reg) set to 1 -> EL1 uses aarch64; 0 -> all low levels are aarch32
                            // aarch64: the 64-bit execution state; aarch32: the 32-bit execution state
                            // The PE can change the execution state only at reset or on a change of EL.
    msr     hcr_el2, x0     // hcr_el2: Hypervisor Configuration Register for EL2
    mov     x0, 0x3c5       // EL1h (SPSel = 1) with interrupt disabled
                            // bit[9](PSTATE.D): the debug exception mask bit, bit[8](PSTATE.A): SEerror mask bit
                            // bit[7](PSTATE.I): the IRQ mask bit, bit[6](PSTATE.F): The FIQ mask bit
                            // bits[3:2](PSTATE.EL): The Exception Level; bit[0](PSTATE.SP): The stack pointer selection bit 
    msr     spsr_el2, x0    // Saved Program Status Register for EL2
                            // On return from the exception, restore the PE state to the state stored
                            // in the SPSR at the Exception level the exception is returning from
    mov		x0, #0x40000    // put EL1 stack start from 0x40000	
    msr	    SP_EL1, x0      // By default, taking an exception selects the stack pointer for the target Exception level, SP_ELx
    msr     elr_el2, lr     // elr: exception link registers (for the preferred return address)
    eret                    // exception return to EL1

// mrs: Load value from a system register to one of the general purpose registers (x0–x30)
// and: Perform the logical AND operation. We use this command to strip the last byte from the value we obtain from the mpidr_el1 register.
// cbz: Compare the result of the previously executed operation to 0 and jump (or branch in ARM terminology) to the provided label if the comparison yields true.
// b: Perform an unconditional branch to some label.
// adr: Load a label's relative address into the target register.
// sub: Subtract values from two registers.
// bl: "Branch with a link": perform an unconditional bra/nch and store the return address in x30 (the link register). When the subroutine is finished, use the ret instruction to jump back to the return address.
// mov: Move a value between registers or from a constant to a register.
// ldr: load data from memory into a register
// str: store (write) a value from a register into memory at a specified address.

