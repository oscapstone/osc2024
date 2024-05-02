.section ".text.boot"
.globl _start_boot

_start_boot:
    ldr x19, =_dtb
    str x0, [x19]
    
    mrs    x20, mpidr_el1        
    and    x20, x20,#0xFF 
    cbz    x20, master  

hang:
    b hang

master:
    adr    x20, bss_begin
    adr    x21, bss_end
    sub    x21, x21, x20
    bl     memzero

    mov   sp, #0x3F000000
    bl    bootloader_main

memzero:
	str xzr, [x20], #8
	subs x21, x21, #8
	b.gt memzero
	ret
    
.global _dtb
.section .data
_dtb: .dc.a 0x0
