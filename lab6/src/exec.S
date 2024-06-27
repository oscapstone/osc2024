
.globl enter_el0_run_user_prog
enter_el0_run_user_prog:
    // Set exception return address
    msr elr_el1, x0

    // Set user stack
    msr sp_el0, x1

    // Enable interrupt ({D, A, I, F} = 0 (unmasked))
    // EL0 ({M[3:0]} = 0)
    mov x0, 0
    msr spsr_el1, x0

    // TODO: Clear all general registers
    
    // return to EL0
    eret

.globl exec_user_prog
exec_user_prog:
    // Set exception return address
    msr elr_el1, x0

    // Set user stack
    msr sp_el0, x1

    // Enable interrupt ({D, A, I, F} = 0 (unmasked))
    // EL0 ({M[3:0]} = 0)
    mov x0, 0
    msr spsr_el1, x0

    // Set kernel stack
    mov sp, x2

    // TODO: Clear all general registers
    
    // return to EL0
    eret