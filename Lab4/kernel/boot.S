#include "../peripherals/mm.h"

.section ".text.boot"

.globl _start
_start:
  // mrs: Move the contents of a special register to a general-purpose register.
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
  // Transit exception level before running kernel code.(EL2 -> EL1)
  // Has to transit from the start.
  bl el2_to_el1

  
  //  Initialize stack pointer.
  //  Heap end address: 0x1195B88
  //  Allocate 1MB for stack: 0x1195B88 + 0x100000 = 0x1295B88
  //  Align to 16-byte boundary.
  ldr x0, =0x1295B90
  mov sp, x0
    
  // Calculate the size of the bss section(stores uninitialized data)
  // adr loads the address of the label into the register.
  ldr	x0, =bss_begin
  ldr	x1, =bss_end
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

// Change the exception level from EL2 to EL1 for the kernel.(By default, RPi's CPU runs
// in EL2 mode after booting)
el2_to_el1:
  // EL1 uses aarch64
  // Consult https://developer.arm.com/documentation/ddi0601/2021-06/AArch64-Registers/HCR-EL2--Hypervisor-Configuration-Register?lang=en
  // for setting the register.
  mov x0, (1 << 31)

  // msr: Move the contents of a general-purpose register into the specified special register.
  // The hcr_el2(Hypervisor Configuration Register) provides configuration controls for virtualization.
  // It's used in hypervisor mode(EL2) to control various aspects of system behavior when executing lower
  // exception levels.
  msr hcr_el2, x0

  // https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/SPSR-EL2--Saved-Program-Status-Register--EL2-
  // Set SPSR_EL2(Saved Program Status Register)
  // EL1h (SPSel = 1) with interrupt disabled
  // [3:0] 0b0101: EL1 with SP_EL1 (EL1h) -> AArch64 Exception level and selected Stack Pointer.
  // [6]: FIQ interrupt mask.
  // [7]: IRQ interrupt mask.
  // [8]: SError interrupt mask.
  // [9]: Debug exception mask.
  mov x0, 0x3C5

  // SPSR_EL2 holds the saved processor state when an exception is taken to EL2. Set the spsr_el2 register
  // to ensure correct PSTATE when returning to EL1.
  msr spsr_el2, x0

  // Set ELR_EL2(Exception Link Register) from LR(Link Register)
  // Holds the return address when executing an exception return operation(eret).
  msr elr_el2, lr
  
  eret // return to EL1

// Prevents other CPU cores from proceeding any further.
proc_hang:
  b proc_hang

.section .data
low_stack_top:  .word 0x1295B00
