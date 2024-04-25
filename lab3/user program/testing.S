.section ".text"              // Directs the assembler to place the following code in the ".text" section
.global _start                // Makes _start globally accessible, typically the entry point in bare-metal programming
_start:                       // Label marking the start of the executable code

    mov x0, 0                 // Initializes register x0 to 0; x0 will be used as a loop counter

1:                             // Label "1" marks the beginning of the first loop
    add x0, x0, 1             // Increments the value in x0 by 1
    svc 0                     // Performs a supervisor call with immediate value 0 (used here for demonstration)
    cmp x0, 5                 // Compares the value in x0 to 5
    blt 1b                    // Branches back to label "1" if x0 is less than 5, continuing the loop

1:                             // Another label "1", re-used for clarity, marking the start of an infinite loop
    b 1b                      // Unconditionally branches to itself, creating an infinite loop
