#include"header/exec.h"
#include"header/malloc.h"

void exec(char *testing){
    // allocate space for executing the input program
    char *ustack = simple_malloc(2000);
    // goto el0 to run program

    //uses the msr (Move to System Register) instruction to set the 
    //SPSR_EL1 (Saved Program Status Register at Exception Level 1) to the value of xzr (zero register). 
    //This effectively clears the SPSR_EL1, resetting any flags (like condition flags or interrupt masks) it holds. 
    // spsr_el1 
    // 0~3 bit 0b0000 : el0t , jump to el0 and use el0 stack
    // 6~9 bit 0b0000 : turn on every interrupt
    asm volatile("msr spsr_el1, xzr\n\t");
    //writes the value in the variable testing to the ELR_EL1 (Exception Link Register at Exception Level 1) register. 
    //The ELR_EL1 holds the return address for exceptions, typically used when returning from an exception back to the 
    //main execution flow. 
    //The value in testing is expected to be a memory address 
    //where execution should resume upon executing the eret instruction later.
    // elr_el1 set program start address
	asm volatile("msr elr_el1, %0\n\t"
	::"r"(testing));
    //sets the SP_EL0 (Stack Pointer for Exception Level 0, set at top) register to the address ustack + 2000. 
    //This setup is preparing the stack pointer for use in EL0, usually after an exception return. 
    //The +2000 likely ensures that there's adequate space allocated in the stack 
    //to handle whatever process will run at EL0, avoiding stack overflow.
	asm volatile("msr sp_el0, %0\n\t"::
        "r"(ustack  + 2000));
    //return from an exception. This instruction uses the value of ELR_EL1 to set the program counter 
    // and SPSR_EL1 to restore previously saved processor state. 
    //Essentially, this transfers control to the address specified in ELR_EL1, effectively resuming execution at the point 
    //decided by earlier instructions.
	asm volatile("eret\n\t" );
	//stack will keep growing
}
