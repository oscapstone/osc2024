.section ".text.boot"

.global _start

// Entry point definition
_start:
    bl from_el2_to_el1               // Transition from EL2 to EL1

// Set the exception vector table
set_exception_vector_table:
    adr x1, exception_vector_table  // Get the address of the exception vector table
    msr vbar_el1, x1                // Set the Vector Base Address Register for EL1

// Set up the stack top
setup_stack:
    ldr x1, =_stack_top             // Load the address of the stack top
    mov sp, x1                      // Set the stack pointer

// Initialize BSS segment
setup_bss:
    ldr x1, =_bss_top               // Load the start address of the BSS segment
    ldr w2, =_bss_size              // Load the size of the BSS segment

init_bss:
    cbz w2, run_main                // If BSS size is zero, jump directly to the main program
    str xzr, [x1], #8               // Zero out the BSS segment
    sub w2, w2, #1                  // Decrement the BSS size by 1
    cbnz w2, init_bss               // If BSS is not fully initialized, continue initialization

// Run the main program
run_main:
    bl main                         // Call the main function

// Loop hang process
proc_hang:
    wfe                             // Wait for event
    b proc_hang                     // Infinite loop

// Implementation of the transition from EL2 to EL1
from_el2_to_el1:
    mov x1, (1 << 31)               // Set the RW bit in HCR_EL2, ensuring EL1 is in AArch64 execution state
    msr hcr_el2, x1
    mov x1, 0x3c5                   // Set SPSR_EL2, specifying the return state as EL1h, with DAIF disabled
    msr spsr_el2, x1
    msr elr_el2, lr                 // Set the return address
    eret                            // Execute exception return, transitioning back from EL2 to EL1
