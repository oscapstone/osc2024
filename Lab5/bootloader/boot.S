#include "../peripherals/mm.h"

.section ".text.boot"

.global _start

_start:
    // The initial state of the registers x0, x1, x2, and x3 upon startup holds specific information
    // passed by the firmware/bootloader.
    // x0: The 64-bit machine model number (from the Device Tree Blob, or DTB). This is an identifier
    // for the hardware model the software is running on, which can be used to adjust behaviors or
    // operations for different hardware variants if necessary.
    mov     x10, x0
    // x1: The physical address of the Device Tree Blob (DTB) in memory. The DTB is a binary
    // representation of the device tree, which describes the hardware components of the system in a
    // tree-like structure. This includes information about the processor, memory, peripherals, and
    // other hardware components. The operating system or bootloader can parse the DTB to dynamically
    // discover the hardware configuration of the system.
    mov     x11, x1
    // x2: Reserved or unused in the standard Raspberry Pi boot process. Its value may not be defined
    // or may be specific to certain configurations or future use. It's often safe to assume this register
    // does not hold information critical to the initial boot process unless specified by new documentation
    // or specific boot configurations.
    mov     x12, x2
    // x3: Reserved or unused, similar to x2. Its purpose is dependent on the booting firmware or specific
    // use cases and is generally not used in the standard boot process for Raspberry Pi.
    mov     x13, x3

// Relocate the boatloader(0x80000) to another location(0x60000) to avoid overlapping with kernel.
self_relocate:
    ldr x2, =0x60000
    adr x0, __bootloader_start
    adr x1, __bootloader_end
    sub x1, x1, x0

relocate:
    ldr x3, [x0], #8
    str x3, [x2], #8
    subs x1, x1, #8
    b.gt relocate


// Initialize the bss section.
clear_bss:
    // Initialize stack pointer.
    mov sp, #LOW_MEMORY
    adr x0, __bss_start
    adr x1, __bss_end
    sub x1, x1, x0

// Iterate through the bss section and initialize it with zeros.
memzero:
    // Stores the value from the zero register(xzr) to the memory location pointed to
    // by x0. After storing the bytes, increment x0 by 8 bytes, since it's a 64-bit
    // architecture.
    str xzr, [x0], #8

    // x1 contains the size of bss that needs to be zeroed out. subs also updates
    // the condition flags based on the result.
    subs x1, x1, #8

    // It's a conditional branch that jumps back to memzero label if the condition
    // flags indicate that the result of the previous "subs" was greater than zero.
    b.gt memzero
    bl bootloader_main - 0x20000

// This step shouldn't be reached, define it just in case.
proc_hang:
    b proc_hang
