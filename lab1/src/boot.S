#include "mm.h" // Include memory management header

.section ".text.boot" // Specify the section for boot code

.globl _start // Declare the entry point of the program
_start:
    mrs x0, mpidr_el1       // Read the Multiprocessor Affinity Register
	//mrs Load value from a system register to one of the general purpose registers (x0–x30)
    and x0, x0, #0xFF       // Isolate the CPU ID by masking with 0xFF
	//and Perform the logical AND operation. We use this command to strip the last byte from the value we obtain from the mpidr_el1 register.
    cbz x0, master          // If CPU ID is 0 (primary CPU), jump to 'master' label
	//cbz Compare the result of the previously executed operation to 0 and jump (or branch in ARM terminology) to the provided label if the comparison yields true.
    b   proc_hang           // For non-primary CPUs, jump to 'proc_hang'
	b Perform an unconditional branch to some label.

proc_hang:
    b   proc_hang           // Infinite loop to hang the non-primary CPUs

master:
    adr x0, bss_begin       // Load the address of the beginning of the BSS segment into x0
	//adr Load a label’s relative address into the target register. In this case, we want pointers to the start and end of the .bss region.
    adr x1, bss_end         // Load the address of the end of the BSS segment into x1
    sub x1, x1, x0          // Calculate the size of the BSS segment
	//sub Subtract values from two registers.
    bl  memzero             // Call memzero function to zero out the BSS segment
	//bl “Branch with a link”: perform an unconditional branch and store the return address in x30 (the link register). When the subroutine is finished, use the ret instruction to jump back to the return address.
    mov sp, #LOW_MEMORY     // Set the stack pointer to the start of low memory
	//mov Move a value between registers or from a constant to a register.
    bl  kernel_main         // Call the kernel's main function
    b   proc_hang           // Should never reach here, but hang if it does
