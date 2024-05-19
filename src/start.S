#include "vm_macro.h"

.section ".text.boot"

.global _start 

_start:
    mrs     x1, mpidr_el1       // Multiprocessor Affinity Register: get cpu id and put to reg x1
    and     x1, x1, #0x3        // Keep the lowest two bits
    cbz     x1, setting         // if cpu_id > 0, stop
proc_hang:
    wfe                     
    b       proc_hang
setting:                            
    mov     x27, x0
    ldr     x1, =SCTLR_VALUE_MMU_DISABLED   // diable MMU
    msr     sctlr_el1, x1
    bl      from_el2_to_el1
    bl      set_exception_vector_table
    bl      setup_virt
    bl      setup_pagetable
    ldr     x1, =SCTLR_VALUE_MMU_ENABLE     // enable MMU
    mrs     x2, sctlr_el1
    orr     x2, x2, x1
    msr     sctlr_el1, x2
    ldr     x1, =KERNEL_VIRT_BASE
    add     sp, sp, x1
    ldr     x1, =__bss_start                // clear bss
    ldr     x2, =__bss_size
clear_bss:  
    cbz     x2, master                      // if val in reg x2 is zero, thn jump to setup, indicating that the .bss section has been zeroed.
    str     xzr, [x1], #8                   // write a zero value (from the xzr register) to the memory address pointed to by x1, and then increment the value of x1, effectively zeroing the next 8 bytes.
    sub     x2, x2, #1
    cbnz    x2, clear_bss
master:  
    mov     x0, x27
    bl      main            // main function
    b       proc_hang       // halt this core if return

from_el2_to_el1:
    mov     x0, (1 << 31)           // RW, bit[31] (in HCR_EL2 reg) set to 1 -> EL1 uses aarch64; 0 -> all low levels are aarch32
                                    // aarch64: the 64-bit execution state; aarch32: the 32-bit execution state
                                    // The PE can change the execution state only at reset or on a change of EL.
    msr     hcr_el2, x0             // hcr_el2: Hypervisor Configuration Register for EL2
    mov     x0, 0x3c5               // EL1h (SPSel = 1) with interrupt disabled
                                    // bit[9](PSTATE.D): the debug exception mask bit, bit[8](PSTATE.A): SEerror mask bit
                                    // bit[7](PSTATE.I): the IRQ mask bit, bit[6](PSTATE.F): The FIQ mask bit
                                    // bits[3:2](PSTATE.EL): The Exception Level; bit[0](PSTATE.SP): The stack pointer selection bit 
    msr     spsr_el2, x0            // Saved Program Status Register for EL2
                                    // On return from the exception, restore the PE state to the state stored
                                    // in the SPSR at the Exception level the exception is returning from
    ldr		x0, =0x80000            // put EL1 stack start from the top of the kernel
    msr	    sp_el1, x0              // By default, taking an exception selects the stack pointer for the target Exception level, SP_ELx
    msr     elr_el2, lr             // elr: exception link registers (for the preferred return address)
    eret                            // exception return to EL1

setup_virt:
    ldr     x0, =TCR_EL1_VALUE  
    msr     tcr_el1, x0         // tcr_el1: Translation Control Register (EL1)
    ldr     x0, =MAIR_EL1_VALUE 
    msr     mair_el1, x0        // mair_el1: Memory Attribute Indirection Register (EL1). Provides the memory attribute encodings corresponding 
    ret                         // to the possible AttrIndx values in a Long-descriptor format translation table entry for stage 1 translations at EL1.
                                
setup_pagetable:
    /*
     * Each page table base address:
     *  x0: PGD (1 page, 1 entry)
     *  x1: PUD (1 page, 2 entry)
     *  x2: PMD (1 page, 512 entry)
     *  x3: PTE (512 page, each with 512 entry)
    */
    ldr     x0, =__pt_start           // PGD's page frame address
    lsl     x0, x0, #16               // omit first 16 bits (ffff) (using physical address)
    lsr     x0, x0, #16               // Shift right by 16 bits
    add     x1, x0, #PAGE_SIZE        // PUD's page frame address
    add     x2, x1, #PAGE_SIZE        // PMD's page frame address

    // set up PGD and PUD
    ldr     x3, =BOOT_PGD_ATTR
    orr     x3, x1, x3              // combine the physical address of next level page with attribute.
    str     x3, [x0]
    ldr     x3, =BOOT_PUD0_ATTR
    orr     x3, x2, x3
    str     x3, [x1]                // 1st 1GB mapped by the 1st entry of PUD
    ldr     x3, =BOOT_PUD1_ATTR
    mov     x4, 0x40000000
    orr     x3, x4, x3
    str     x3, [x1, 8]             // 2nd 1GB mapped by the 2nd entry of PUD

    // set up PMD
    add     x3, x2, #PAGE_SIZE        // PTE0's page frame address
    mov     x4, x2                    // PMD's page frame address
    mov     x6, #512
1:
    ldr     x5, =BOOT_PMD_ATTR
    orr     x5, x3, x5                 // PTE physical address
    str     x5, [x4], #8
    add     x3, x3, #PAGE_SIZE
    sub     x6, x6, #1
    cbnz    x6, 1b

    // set up PTE for normal memory
    mov     x3, xzr                 
    add     x4, x2, #PAGE_SIZE        // PTE0's page frame address
    mov     x6, 0x3F000               // 0x3C000 PTEs for ARM memory size
2:
    ldr     x5, =PTE_NORAL_ATTR
    orr     x5, x3, x5
    str     x5, [x4], #8
    add     x3, x3, #PAGE_SIZE
    sub     x6, x6, #1
    cbnz    x6, 2b

    // set up PTE for device memory
    mov     x6, 0x40000
3:
    ldr     x5, =PTE_DEVICE_ATTR
    orr     x5, x3, x5
    str     x5, [x4], #8
    add     x3, x3, #PAGE_SIZE
    sub     x6, x6, #1
    cbnz    x6, 3b

    msr ttbr0_el1, x0           // load PGD to the bottom translation-based register.
    msr ttbr1_el1, x0           // ttbr`i`_el1 is selected when the upper bits of the VA are all set to `i`
    ret

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
// wfe: wait-for-event instruction, which makes processors and is targeted at entering either standby, dormant or shutdown mode, where an interrupt is required to wake-up the processor.
