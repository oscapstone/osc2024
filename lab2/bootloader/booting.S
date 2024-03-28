.section ".text.boot"     // Start a new section named ".text.boot"
.global _start            // Declare _start symbol as global

_start:                   // Start of the _start block
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1 // Read Multiprocessor Affinity Register into x1
    and     x1, x1, #3    // Mask the lower 2 bits (CPU core ID) from x1
    cbz     x1, setting   // If x1 is zero, jump to setting label

halt:   wfe                // Wait for Event instruction
        b   halt           // Branch (jump) to halt (infinite loop)

setting:                  // Setting label
    ldr x1, =_start       // Load the address of _start into x1
    mov sp, x1            // Move the value of x1 into the Stack Pointer (sp)
    ldr x1, =__bss_start  // Load the address of __bss_start into x1
    ldr w2, =__bss_size   // Load the value of __bss_size into w2

clear_bss:                // Clear BSS segment loop label
    cbz w2, kernel_main   // If w2 (BSS size) is zero, jump to kernel_main
    str xzr, [x1], #8     // Store zero (xzr) at the address pointed by x1, then increment x1 by 8
    sub w2, w2, #1         // Decrement w2 (BSS size) by 1
    cbnz w2, clear_bss    // If w2 is not zero, jump back to clear_bss

kernel_main:              // Label for the start of kernel_main function
    bl main               // Branch with link (call) to main function
    b halt                 // Branch to halt (infinite loop)
