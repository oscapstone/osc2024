# Lab3
## Exception
### Exception Level
* Firmware: EL3
* Default while booted: EL2
* OS: EL1
* Program: EL0

### Exception Handling
**Spec**

When a CPU takes an exception, it does the following things.
* Save the current processor’s state(PSTATE) in SPSR_ELx. (x is the target Exception level)
* Save the exception return address in ELR_ELx.
* Disable its interrupt. (PSTATE.{D,A,I,F} are set to 1).
* If the exception is a synchronous exception or an SError interrupt, save the cause of that exception in ESR_ELx.
* Switch to the target Exception level and start at the corresponding vector address.

After the exception handler finishes, it issues eret to return from the exception. Then the CPU,
* Restore program counter from ELR_ELx.
* Restore PSTATE from SPSR_ELx.
* Switch to the corresponding Exception level according to SPSR_ELx.

### EL2 to EL1
[hcr_el2](https://blog.csdn.net/heshuangzong/article/details/127695422)
[spsr_el2](https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/SPSR-EL2--Saved-Program-Status-Register--EL2-)
```
//MSR（Move to System Register）
from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64 (set the bit 31)
    msr hcr_el2, x0 // this means el1 will use aarch64, see link
    mov x0, 0x345 // 3c5 EL1h (SPSel = 1) with interrupt disabled, 345 enable interrupt
    msr spsr_el2, x0 //1111000101 or 1101000101
    //[0:3] selected level 101 -> EL1 with SP_EL1 (EL1h) 9876 DAIF 7: Interrupt
    msr elr_el2, lr //address to go after return, link register is selected here
    eret // return to EL1 (return from exception, check setting of spsr_el2)
```

### EL1 to EL0
[spsr_el1](https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/SPSR-EL1--Saved-Program-Status-Register--EL1-)
```
asm volatile ("mov x0, 0x3c0"); // [3:0] select el0
asm volatile ("msr spsr_el1, x0"); 
asm volatile ("msr elr_el1, %0": :"r" (current)); //eret to user program
asm volatile ("mov x0, 0x20000");
asm volatile ("msr sp_el0, x0"); // place stack pointer for program
asm volatile ("eret");
```

#### Entry Output:

* SPSR_EL1: 0x00000000000003C5
* ELR_EL1: 0x0000000008010F8C

**ESR_EL1: 0x0000000056000000**

EC: Exception Class. Indicates the reason for the exception that this register holds information about.
IL: Instruction Length for synchronous exceptions.

* EC [31:26] -> 010101 (from 56): SVC instruction execution in AArch64 state.
* IL [25] -> 1 (synchronous)

[ARM Document](https://developer.arm.com/documentation/ddi0601/2020-12/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-)

### Vector Table
![image](https://hackmd.io/_uploads/SJ0VHlXgR.png)

Exceptions in this lab:
* (EL1 -> EL1) Exception from the currentEL while using SP_ELx 
* (EL0 -> EL1) Exception from a lower EL and at least one lower EL is AARCH64. (Synchronous(SVC))