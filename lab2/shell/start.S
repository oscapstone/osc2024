.section ".text.boot"

.global _start

//bootloader會在kernel開始執行時將DTB的位址傳遞到 x0 reg.中

_start:
    ldr	x1, =_dtb_ptr	// 把_dtb_ptr載入x1
    
    // 將DTB的位址存到 _dtb_ptr，以供kernel使用
    // 因為kernel需要了解有關系統硬體的詳細資訊
    str	x0, [x1]	// 把x0的值存入_dtb_ptr
    
    
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0, stop
    
1:  
    wfe
    b   1b


// setting 
2:  
    // cpu id == 0
    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
    
    
3:  
    cbz     w2, 4f        // if w2 == 0, 4
    str     xzr, [x1], #8 // xzr是零reg, 把0存入x1, 目的是將bss清空
    sub     w2, w2, #1
    cbnz    w2, 3b

    
4:
    // jump to C code, should not return
    bl      main
    // for failsafe, halt this core too
    b       1b

.global _dtb_ptr	// define a global variable _dtb_ptr
.section .data		// _dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0	// it defines _dtb_ptr to be a 8-byte constant with a value of 0x0
