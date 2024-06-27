.section .text._start

_STACK_START                = 0xffff000000070000
_CS_COUNTER_ADDRESS         = 0x75000
_DEVICE_TREE_ADDRESS        = 0x75100

.global _start
.global _bss_start
.global _bss_end

.macro ADR_REL register, symbol
  adrp  \register, \symbol
  add  \register, \register, #:lo12:\symbol
.endm

_start:
    ldr x1, =_DEVICE_TREE_ADDRESS
    str x0, [x1]

    // Clear CS counter
    ldr x0, =_CS_COUNTER_ADDRESS
    str xzr, [x0]

    // Change EL
    bl .from_el2_to_el1

    mrs x0, CurrentEL
    cmp x0, #0x4 // Check if we are in EL1
    bne .from_el2_to_el1

    // Initailize stack pointer
    ldr x0, =0x70000
    mov sp, x0

    bl setup_mmu

    ldr x1, = boot_rest // indirect branch to the virtual address
    br x1

boot_rest:
    // Initailize stack pointer
    ldr x0, =_STACK_START
    mov sp, x0
    
    // Initialize bss
    ADR_REL x0, _bss_start
    ADR_REL x1, _bss_end
.L_clear_bss:
    cmp x0, x1
    bge .L_done_clearing
    str xzr, [x0], #8
    b .L_clear_bss
.L_done_clearing:

    // Initialize exception vector table
    adr x0, exception_vector_table
    msr vbar_el1, x0

    // Call rust main function
    b   _start_rust

.from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    mov x0, sp
    msr sp_el1, x0
    eret // return to EL1

.size	_start, . - _start
.type	_start, function
