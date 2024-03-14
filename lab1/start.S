.section ".text.boot"
.global _start

_start: 
    //0x80000;
    mrs	x1, mpidr_el1
    // read cpu id, stop slave cores
    and	x1, x1, #3
    // four-core cpu, & with 11 to preserve latest two bits
    cbz	x1, setting
    // cbz: compare branch zero, if x1==0 jump to setting; otherwise continue



// if x1 != 0, go to hang and wait
halt:
    wfe
    b halt
  
  
  
setting: 
    ldr x1, =_start 
    //load data from _start into register x1
    mov sp, x1
    //move a value x1 into a register sp
    ldr x1, =__bss_start
    ldr w2, =__bss_size
    //__bss_start and __bss_size define in linker file
    // x1 is 64bits for address, w2 is 32 bits for value

kernel_main: 
    bl main
    b  halt
