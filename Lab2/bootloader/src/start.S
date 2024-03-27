.section ".text.relo"

.global _start

_start:
	// relocate bootloader
	ldr x1, =0x80000
	ldr x2, =__text_start
	ldr x3, =__bootloader_size

relocate:
	ldr x4,[x1],#8 //store value from x1 to x4 than mov x1
	str x4,[x2],#8 //store value from x4 to x2 than mov x2
	sub x3,x3,#1
	cbnz x3, relocate

end_relo:
    ldr x14, =_bl_entry    // 載入 _bl_entry 變數的地址，存入 x14 中
    br x14               // 跳轉到 x14 中存儲的地址，即引導程序的入口地址

.section ".text.boot"

.global _start_bl

_start_bl:
    ldr     x1, =_dtb_ptr
    str     x0, [x1]
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3  //Check processor id
    cbz     x1, setting      //if CPU id = 0, jump to 2f
    b       hang

hang:  wfe
    b       hang

setting:  ldr     x1, =_start
    mov     sp, x1
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

clear_bss:  cbz     w2, master
    str     xzr, [x1], #8  // x1* = xzr, x1 += 8 (write xzr represents discard, read xzr == 0)
    sub     w2, w2, #1
    cbnz    w2, clear_bss

master:  bl      main



.global _dtb_ptr	//define a global variable _dtb_ptr
.section .data		//_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	//it defines _dtb_ptr to be a 8-byte constant with a value of 0x0