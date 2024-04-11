
.section ".text.kernel"

.global _start



_start:
    ldr	    x1, =_dtb_ptr   //put _dtb_ptr into register1
    str	    x0, [x1]	    //store dtb address from x0 to _dtb_ptr
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0, stop
    
1:  
    wfe
    b  1b
     
2:  
    // cpu id == 0
    ldr x1, =_start
    bl	from_el2_to_el1
    
    mov sp, x1
    
    
    /* set exception vector table */
    // Set the vector table to implement the exception handler(in EL1)
    // vbar_el1 : Vector Base Address Register (EL1) 
    // 任何發生在EL1的exception, 儲存vector的base addr
    // 將 exception_vector_table的位址寫入 vbar_el1 暫存器中
    // 告訴處理器在 EL1 模式下發生exception時應該跳到哪裡執行exception handler。
    adr	x0, exception_vector_table
    msr	vbar_el1, x0
    
    bl core_timer_enable
    // clear bss
    ldr x1, =__bss_start
    ldr w2, =__bss_size

3:  
    cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b
    // bl core_timer_enable
    // jump to C code, should not return

4:  
    bl      main
    // for failsafe, halt this core too
    b       1b


//Basic1

from_el2_to_el1:

    // hcr_el2 : Hypervisor configuration register(EL2)
    // This register controls various aspects of the hypervisor's configuration and operation when the processor is in EL2
    // 將hcr_el2 第32bit設為1代表processor在EL1執行的status是AArch64 mode
    // 第32bits確定execution state
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    
    // spsr_el2 : Saved program status register
    // 當EL2發生exception, spsr會保存processor的狀態, 用於當執行eret後恢復processor的狀態
    // 0x3c5 = 0b1111000101
    // 這裡希望將EL1設定為EL1h mode(0b0101)並disabled interrupt
    // bit9 : Endianness
    // bit8 : disables the SError (System Error) interrupts.
    // bit7 : disables the IRQ (Interrupt Request) interrupts.
    // bit6 : disables FIQ (Fast Interrupt Request) interrupts.
    mov x0, 0x3c5     // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    
    // elr_el2 : exception link register
    // 在EL2的exception return, 用來保存processor的return addr
    // 執行eret時, processor使用elr_el2中儲存的val當作addr恢復執行
    // 透過將elr_el2設定為linker reg中的值, 
    // 告訴processor在return EL1後從lr儲存的值恢復執行
    msr elr_el2, lr
    
    // eret : exception return to EL1
    eret




// Basic2
// enable the core timer’s interrupt.
core_timer_enable:
	mov x0,1
	
	// cntp_ctl_el0 : counter-timer physical timer control register
	// el1 physical timer的control reg
	// 0 : disabled, 1 : enable
	msr cntp_ctl_el0, x0 //enable interrupt
	
	// cntfrq_el0 : the frequency of the timer
	// cntfrq_el0儲存timer運行的頻率，即CPU頻率。
	mrs x0, cntfrq_el0
	
	// reg設置timer的expired time
	msr cntp_tval_el0,x0 // set expired time
	// unmask
	mov x0,2
	// core timer interrupt controller addr
	ldr x1, =CORE0_TIMER_IRQ_CTRL
	// 取消unmask寫入timer interrupt controller
	str w0,[x1] //unmask timer interrupt

core_timer_handler:
  // 將 cntfrq_el0的值讀取到x0，x0儲存了timer的頻率。
  mrs x0, cntfrq_el0
  // 將x0 中的值寫入 cntp_tval_el0，用於設定timer的timeout。
  msr cntp_tval_el0, x0





// save general registers to stack
// save registers before entering the kernel
.macro save_all
    sub sp, sp, 32 * 8
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]
.endm

// load general registers from stack
// load them before exiting the kernel
.macro load_all
    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 32 * 8
.endm


// 因為user program and the exception handler共享相同general purpose registers bank
// 進入kernel function前先儲存program context
// load them before exiting the kernel.
exception_handler:
    save_all
    bl except_handler_c
    load_all
    eret

irq_exception_handler:
	save_all
	bl irq_except_handler_c
	load_all
	eret


// 2^11 = 2048 bytes, 0x800 = 100000000000
.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
	b exception_handler // branch to a handler function.
	.align 7 // entry size is 0x80, .align will pad 0
	b exception_handler
	.align 7
	b exception_handler
	.align 7
	b exception_handler
	.align 7

	b exception_handler
	.align 7
	b exception_handler
	.align 7
	b exception_handler
	.align 7
	b exception_handler
	.align 7

	b exception_handler
	.align 7
	b exception_handler
	.align 7
	b exception_handler
	.align 7
	b exception_handler
	.align 7

	b exception_handler
	.align 7
	b exception_handler
	.align 7
	b exception_handler
	.align 7
	b exception_handler
	.align 7

.global _dtb_ptr    //define a global variable _dtb_ptr
.section .data	    //_dtb_ptr is in data section
_dtb_ptr: .dc.a 0x0 //it defines _dtb_ptr to be a 8-byte constant with a value of 0x0

