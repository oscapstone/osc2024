.section ".text.boot"

.global _start

_start:
    mrs     x1,  MPIDR_EL1  // get cpu id and put to reg x1
    and     x1, x1, #3      // Keep the lowest two bits
    cbz     x1, setting          // if cpu_id > 0, stop
proc_hang:
    wfe                     
    b       proc_hang
setting:                      // if cpu_id == 0
    ldr		x1, =_dtb_ptr	// put _dtb_ptr into register1
	str		x27, [x1]		// store the dtb address to _dtb_ptr
    ldr     x1, =_start      // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    mov     sp, x1
    ldr     x1, =__bss_start    // clear bss
    ldr     x2, =__bss_size
clear_bss:  
    cbz     x2, master      // if val in reg x2 is zero, thn jump to label 4, indicating that the .bss section has been zeroed.
    str     xzr, [x1], #8   // write a zero value (from the xzr register) to the memory address pointed to by x1, and then increment the value of x1, effectively zeroing the next 8 bytes.
    sub     x2, x2, #1
    cbnz    x2, clear_bss
master:  
    bl      main            // main function
    b       proc_hang              // halt this core if return


.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0


// mrs: Load value from a system register to one of the general purpose registers (x0–x30)
// and: Perform the logical AND operation. We use this command to strip the last byte from the value we obtain from the mpidr_el1 register.
// cbz: Compare the result of the previously executed operation to 0 and jump (or branch in ARM terminology) to the provided label if the comparison yields true.
// b: Perform an unconditional branch to some label.
// adr: Load a label's relative address into the target register. In this case, we want pointers to the start and end of the .bss region.
// sub: Subtract values from two registers.
// bl: "Branch with a link": perform an unconditional bra/nch and store the return address in x30 (the link register). When the subroutine is finished, use the ret instruction to jump back to the return address.
// mov: Move a value between registers or from a constant to a register.
// ldr: load data from memory into a register
// str: store (write) a value from a register into memory at a specified address.


