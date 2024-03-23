.section ".text.boot"

.global _start

_start:
	// relocate bootloader
	ldr x1, =0x80000 
	ldr x2, =0x60000 //x11=0x60000
	ldr w3, =__bootloader_size

relocate:
	ldr x4,[x1],#8 //store value from x1 to x4 than mov x1
	str x4,[x2],#8 //store value from x2 to x4 than mov x2
	sub w3,w3,#1
	cbnz w3,relocate

init_cpu:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1 //get cpu num
    and     x1, x1, #3 //see if cpu num is 0
    cbz     x1, 2f //cpu 0 jump to 2f (others wfe)
    // cpu id > 0, stop

1:  wfe
    b       1b
    
2:  // cpu id == 0

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start //ldr: load Register
    mov     sp, x1 //stack pointer before _start

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
    
3:  cbz     w2, 4f //see if bss is cleared
    str     xzr, [x1], #8 //xzr means zero, clear to zzero from start
    sub     w2, w2, #1 //1 count means 8bit
    cbnz    w2, 3b //if not zero go back

    // jump to C code, should not return

4:  bl      bootloader_main-0x20000
    // for failsafe, halt this core too
    b       1b