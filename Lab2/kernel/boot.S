#include "../peripherals/mm.h"

.section ".text.boot"

.globl _start
_start:
  // mrs: system coprocessor register to ARM register
  // Reads the Multiprocessor Affinity Register (MPIDR_EL1) into register x0.
  // This register contains the processor's unique ID, which is used to identify the core on which the code is running.
  mrs x0, mpidr_el1

  // Performs a bitwise AND operation on x0 with 0xFF to isolate the lower 8 bits 
  // of the MPIDR_EL1 register, which represent the processor ID.
  and x0, x0, #0xFF

  // cbz: Compare and branch on zero
  // If x0 is zero, it indicates the primary or master CPU and branches to the "master" label if true.
  // If x0 is not zero(indicating secondary CPUs), it doesn't branch.
  cbz	x0, master

  // This label followed by an instruction creates an infinite loop, effectively hanging or stopping
  // any secondary CPU cores from proceeding further. This ensures that only the primary core continues
  // execution beyond this point.
  b proc_hang

master:
  // Initialize stack pointer.
  mov sp, #LOW_MEMORY
  
  // Calculate the size of the bss section(stores uninitialized data)
  // adr loads the address of the label into the register.
  adr	x0, bss_begin
  adr	x1, bss_end
  sub	x1, x1, x0
  bl memzero

  // Device tree address has been moved from x0 to x10 within the bootloader.
  mov x0, x10
  bl kernel_main
  b proc_hang

// Initialize the bss section.
memzero:
  // Stores the value from the zero register(xzr) to the memory location pointed to
  // by x0. After storing the bytes, increment x0 by 8 bytes, since it's a 64-bit
  // architecture.
  str xzr, [x0], #8

  // x1 contains the size of bss that needs to be zeroed out. subs also updates
  // the condition flags based on the result.
  subs x1, x1, #8

  // It's a conditional branch that jumps back to memzero label if the condition
  // flags indicate that the result of the previous subs was greater than zero.
  b.gt memzero
  ret

// Prevents other CPU cores from proceeding any further.
proc_hang:
  b proc_hang
