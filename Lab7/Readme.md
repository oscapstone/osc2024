# OSC2024 Lab5

## Thread
### Registers
* tpidr_el1: A register to save the address of current thread. During initialization, place thread[0] to tpidr_el1 ensures that when switching, main process will be saved to the thread structure.
* sp: the place of current stack (sp_el0 in el0 and sp_el1 in el1)
* fp: the function frame address for local variable, set by function call (backtrace) (or the top of stack, I think replaced by sp, I've tried not to save and works)
* lr: the address to jump to when ret is called (return address of function)
* elr_el1: the address to return after exception handling (usually next instruction before svc)
* ret: jump to lr
* eret: execption return, jump to elr_el1
* spsr_elx: Saved Program Status Register (DAIF, select el, stack)
* sp_elx: the stack pointer address for specific el
* esr_el1: determine the exception source (recall lab3, EC[31:26] -> 010101: svc)

### Caller vs Callee vs Others
* caller: the one to call other function
* callee: the function to be called
#### Caller Saved
* x0~x7: parameter and result registers
* x8: complex return value / svc code

#### Corruptible Registers
* x9~x15: can be used freely without saving them
* x16, x17: for linker/compiler

#### Callee Saved
* x19~x28: callee has to save before use and restore while return
* x29: frame pointer
* x30: lr

#### Others
* x18: Platform Register

### switch_to
Save all callee registers and sp to save and restore thread state.

* x0 is from (store from register to thread struct)
* x1 is to (load from thread to register)
```
.global switch_to
switch_to:
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]

    ldp x19, x20, [x1, 16 * 0]
    ldp x21, x22, [x1, 16 * 1]
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]
    ldr x9, [x1, 16 * 6]
    mov sp,  x9
    msr tpidr_el1, x1
    ret

.global get_current
get_current:
    mrs x0, tpidr_el1
    ret
```
[ ] : get the value of the address, ex: ldr x0, [x1] -> load from address(address is value of x1)
switch_to: no exception handling -> use lr
eret: need exception return -> use elr_el1

x1, 16*n -> x1 + 16 * n -> offset

Note: before switch to will be timer interrupt, so the exit syscall can directly use switch_to, it will go back to handler and eret. 

## User Process and System Call
Trapframe: the connection between user process and kernel space (send and return value)

**Implementation:** see code.

### Timer
Enable user program to directly access timer
```
uint64_t tmp;
asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
tmp |= 1;
asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
```

## Reference
* lr, ret: https://blog.csdn.net/boildoctor/article/details/123379261
* esr_el1: https://developer.arm.com/documentation/ddi0601/2020-12/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-
* spsr_el1: https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/SPSR-EL1--Saved-Program-Status-Register--EL1-
* cntkctl_el1: https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/CNTKCTL-EL1--Counter-timer-Kernel-Control-Register
* caller, callee: https://developer.arm.com/documentation/102374/0101/Procedure-Call-Standard
* mailbox: https://nycu-caslab.github.io/OSC2024/labs/hardware/mailbox.html#mailbox
* fp: https://www.csie.ntu.edu.tw/~sprout/algo2021/homework/hand07.pdf