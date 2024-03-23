.section ".text.boot"

.global _start

// x0 register stores address to dtb
_start:
    // Load the current location into register x20 (address of the current instruction)
    adr     x20, .

    // Load the size of the bootloader (_bl_size) into register x21
    ldr     x21, =_bl_size

    // Calculate the end address of the bootloader by adding _bl_size to the current location
    add     x21, x21, x20

    // Load the start address of the text section (_text_start) into register x22
    ldr     x22, =_text_start

    // Copy the value of _text_start into register x19 for later use (initializing the stack pointer)
    mov     x19, x22

    // Relocate the bootloader itself from 0x80000 to 0x60000.
    // This loop copies the instructions of the bootloader from the current location (0x80000) to the new location (0x60000)
1:
    cmp     x20, x21          // Compare the current location (x20) with the end address of the bootloader (x21)
    b.eq    2f                // If they are equal, jump to label 2f (end of relocation)
    ldr     x23, [x20]        // Load a 64-bit word from the current location (instruction of the bootloader)
    str     x23, [x22]        // Store the loaded word to the new location (relocate the instruction)
    add     x20, x20, #8      // Move to the next instruction (increment by 8 bytes)
    add     x22, x22, #8      // Move to the next location to store the instruction
    b       1b                // Branch back to label 1 (continue relocation)

2:
    // Jump to the ".text._bl" section after relocation
    ldr     x1, =_bl_start    // Load the address of the start of the bootloader section
    br      x1                // Branch to the start of the bootloader code

.section ".text._bl"
    // Initialize stack pointer (sp) to the start address of the text section
    mov     sp, x19           // Load the value of x19 (start address of text section) into the stack pointer (sp)

    // Clear the BSS section
    ldr     x20, =_bss_start  // Load the start address of the BSS section
    ldr     x21, =_bss_size   // Load the size of the BSS section
    add     x21, x21, x20     // Calculate the end address of the BSS section
3:
    cmp     x20, x21          // Compare the current location with the end address of the BSS section
    b.eq    4f                // If equal, exit loop
    str     xzr, [x20]        // Store zero to the current location (clearing BSS)
    add     x20, x20, #8      // Move to the next location in the BSS section
    b       3b                // Branch back to label 3 (continue clearing BSS)

4:
    // Jump to the bootloader main function
    bl      bootloader_main   // Call the bootloader main function
5:
    // Failsafe loop to halt the core in case the main function returns
    b       5b                // Branch back to label 5 (infinite loop)
