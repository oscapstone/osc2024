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

#### Asynchronous Exception
* IRQ
* FIQ
* SError

### EL2 to EL1
* [hcr_el2](https://blog.csdn.net/heshuangzong/article/details/127695422)
* [spsr_el2](https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/SPSR-EL2--Saved-Program-Status-Register--EL2-)
* [CurrentEL](https://developer.arm.com/documentation/ddi0595/2021-12/AArch64-Registers/CurrentEL--Current-Exception-Level)
```
//MSR（Move to System Register）
from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64 (set the bit 31)
    msr hcr_el2, x0 // this means el1 will use aarch64, see link
    mov x0, 0x345 // 3c5 EL1h (SPSel = 1) with interrupt disabled, 345 enable interrupt
    msr spsr_el2, x0 //1111000101 or 1101000101 (PSTATE DAIF mask)
    //[0:3] selected level 101 -> EL1 with SP_EL1 (EL1h) 9876 DAIF 7: Interrupt
    msr elr_el2, lr //address to go after return, link register is selected here
    eret // return to EL1 (return from exception, check setting of spsr_el2)
```
* mrs: move from system register 
* : divide command and operations
* = -> output to, r -> any register, %0 the first variable -> el
```
// print current el to show the result
asm volatile ("mrs %0, CurrentEL" : "=r" (el));
el = el >> 2; [3:2] is el
```
### EL1 to EL0
[spsr_el1](https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/SPSR-EL1--Saved-Program-Status-Register--EL1-)
```
asm volatile ("mov x0, 0x3c0"); // [3:0] select el0
asm volatile ("msr spsr_el1, x0"); 
asm volatile ("msr elr_el1, %0": :"r" (current)); //eret to user program, : output : input (to asm register)
asm volatile ("mov x0, 0x20000");
asm volatile ("msr sp_el0, x0"); // place stack pointer for program
asm volatile ("eret");
```

### EL0 to EL1
exception occured -> vector table(EL1) -> handler -> save all -> exception handler entry -> load all -> eret (back to el0)
```
set_exception_vector_table:
  adr x0, exception_vector_table
  msr vbar_el1, x0 // the address of vector base
  ret
```
```
.align 11 // vector table should be aligned to 0x800 -> 16 entries, each 128 -> 2^11
.global exception_vector_table
exception_vector_table:
  // el0 -> el0
  b exception_handler // branch to a handler function.
  .align 7 // entry size is 0x80, .align will pad 0 -> 2^7 = 8x16
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  
  // el1 -> el1
  b exception_handler 
  .align 7
  b interrupt_handler //uart, timer EL1 -> EL1
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  // el0 -> el1
  b exception_handler //el0 -> el1 exception
  .align 7
  b interrupt_handler //el0 -> el1 interrupt
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  
  // from aarch32
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7
```
```
exception_handler:
    save_all
    bl exception_entry
    load_all
    eret
```

```
.macro save_all
    sub sp, sp, 32 * 8 //move down sp (32 Reg x 8 byte)
    stp x0, x1, [sp ,16 * 0] // 16 -> 8 byte + 8 byte
    stp x2, x3, [sp ,16 * 1] // store pair of register to sp
    ...
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15] //store register to sp
.endm
```

```
.macro load_all
    ldp x0, x1, [sp ,16 * 0] // from stack to register
    ldp x2, x3, [sp ,16 * 1]
    ...
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 32 * 8
.endm
```

### Exception_Entry

**SPSR_EL1: 0x00000000000003C0**

Saved Program Status Register (See el transform) (PSTATE) (Z-bit become 8 because exception handle return 0 i guess)

**ELR_EL1: 0x0000000008010F8C**

The address of the program (return address) // cmp x0, 5

**ESR_EL1: 0x0000000056000000**

Exception Syndrome Register

EC: Exception Class. Indicates the reason for the exception that this register holds information about.

IL: Instruction Length for synchronous exceptions.

* EC [31:26] -> 010101 (from 56): SVC instruction execution in AArch64 state.
* IL [25] -> 1 (synchronous)

[ESR_EL1](https://developer.arm.com/documentation/ddi0601/2020-12/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-)

### Vector Table

Exceptions in this lab:
* (EL1 -> EL1) Exception from the currentEL while using SP_ELx 
* (EL0 -> EL1) Exception from a lower EL and at least one lower EL is AARCH64. (Synchronous(SVC))

## Interrupt (Timer)

[Core0 interrupt register](https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) 
* p7: Register address
* p13: Enable/Disable IRQ
* p16: Source of interrupt: CNTPNSIRQ
NS: Non-Secured
```
cntpct_el0: The timer’s current count.

cntp_cval_el0: A compared timer count. If cntpct_el0 >= cntp_cval_el0, interrupt the CPU core. 

cntp_tval_el0: (cntp_cval_el0 - cntpct_el0). You can use it to set an expired timer after the current timer count.
//(Interrupt when become 0)
```

```
core_timer_enable:
    mov x0, 1
    msr cntp_ctl_el0, x0 // enable
    mrs x0, cntfrq_el0
    msr cntp_tval_el0, x0 // set expired time
    mov x0, 2
    ldr x1, =CORE0_TIMER_IRQ_CTRL //second bit enable
    str w0, [x1] // unmask timer interrupt, only 32 bit
    ret
```
```
void core_timer_handler() {
    unsigned long cntfrq, cntpct;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    asm volatile ("msr cntp_tval_el0, %0" : : "r" (cntfrq * 2));
    asm volatile ("mrs %0, cntpct_el0" : "=r" (cntpct));
    cntpct /= cntfrq;

    uart_puts("Seconds since boot: ");
    uart_int(cntpct);
    uart_puts("\n");
}
```
## UART Interrupt (Peripheral)
https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf

### Enable UART Interrupt
* p12 enable Tx/Rx
* p113 enable AUX interrupt
```
void uart_interrupt(){
    //enable uart tx/rx
    *AUX_MU_IER = 1; //Mini Uart Interrupt Enable (RX) 0-bit
    *AUX_MU_IER |= 0x02; //Tx 1-bit
    
    //enable AUX interrupt
    *IRQS1 |= 1 << 29;
}
```
### Handle UART Interrupt
* p13 IIR bits
```
void interrupt_handler_entry(){
    int core0_irq = *CORE0_INTERRUPT_SOURCE;
    int iir = *AUX_MU_IIR; //Mini Uart Interrupt Identify
    if (core0_irq & 2){ // p16, second bit non-security timer
        core_timer_handler();
    }
    else{ //not timer, IIR bit [0] -> 0: pending
        if ((iir & 0x06) == 0x04) // bit [2:1] -> 10: recieve (& 110 -> 100 read,  110 -> 010 write p13)
            uart_read_handler();
        else //bit [2:1] -> 01: Transmit
            uart_write_handler();
    }
}
```

### Read/Write Handler
* Read: Save the input from UART, handles the command in buffer and copy to kernel when see '\n'
* Write: Put characters to the write buffer, execute the command after the characters are outputted
```
void uart_read_handler() {
    char ch = (char)(*AUX_MU_IO);
    if(ch == '\r'){ //command
        //uart_send('\n');
        uart_read_buffer[read_idx] = '\0';
        read_idx = 0;
        strcpy(uart_read_buffer, async_cmd);
        //uart_puts(uart_read_buffer);
        uart_write_buffer[write_idx] = '\n';
        write_idx++;
        uart_write_buffer[write_idx] = '\r';
        write_idx++;
        //shell(uart_read_buffer);
        uart_read_buffer[read_idx] = '\0';
    }
    else{
        uart_read_buffer[read_idx] = ch;
        uart_write_buffer[write_idx] = ch; 
        //uart_send(uart_read_buffer[read_idx]);
        read_idx++;
        write_idx++;
    }
}

void uart_write_handler(){
    char ch;
    if(write_cur < write_idx){
        *AUX_MU_IO=uart_write_buffer[write_cur];
        ch = uart_write_buffer[write_cur];
        write_cur++;
    }
    if(ch == '\r'){
        shell(async_cmd);
        write_cur = 0;
        write_idx = 0;
        uart_write_buffer[0] = '\0';
    }
}
```

## Timer Multiplexing

### EL1 Interrupt
```
mov x0, 0x345 // enable interrupt (1 is mask)
msr spsr_el2, x0
```
### timer
* callback function: the task to do when expired
* data: the data for callback funct (ex. message)
* expires: time of expiring
```
#define MAX_TIMER 10

typedef void (*timer_callback_t)(char* data);

struct timer{
    timer_callback_t callback;
    char data[BUFFER_SIZE];
    unsigned long expires;
};

struct timer timers[MAX_TIMER];
```
### add_timer
1. check is there spare timer by the expire time
2. allocate a timer with the expire time (current + after)
3. check is there a timer with lower expire time, if none, set interrupt
```
void add_timer(timer_callback_t callback, char* data, unsigned long after){
    asm volatile("msr DAIFSet, 0xf");
    int i;
    int allocated = 0;
    unsigned long cur_time = get_current_time();
    unsigned long print_time = cur_time + after;
    for(i=0; i<MAX_TIMER; i++){
        if(timers[i].expires == 0){
            timers[i].expires = print_time;
            int j = 0;
            while(*data != '\0'){
                timers[i].data[j] = *data;
                data++;
                j++;
            }
            timers[i].data[j] = '\0';
            //uart_puts("timer data: ");
            //uart_puts(timers[i].data[j]);
            timers[i].callback = callback;
            allocated = 1;
            break;
        }
    }

    if(allocated == 0){
        uart_puts("Timer Busy\n");
    }

    int new_irq = 1;
    unsigned long min_time = print_time;
    for(int i=0; i<MAX_TIMER; i++){
        if(timers[i].expires < min_time && timers[i].expires > 0){
            new_irq = 0;
            break;
        }
    }

    if(new_irq){
        set_timer_interrupt(print_time - cur_time);
    }
    uart_puts("Seconds to print: ");
    uart_int(print_time);
    uart_puts("\n");
    asm volatile("msr DAIFClr, 0xf");
}
```
### timer.c
```
unsigned long get_current_time(){
    unsigned long cntfrq, cntpct;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    asm volatile ("mrs %0, cntpct_el0" : "=r" (cntpct));
    cntpct /= cntfrq;
    return cntpct;
}

void set_timer_interrupt(unsigned long second){
    //enable timer
    unsigned long ctl = 1;
    asm volatile ("msr cntp_ctl_el0, %0" : : "r" (ctl));
    //set next interrupt by second x freqeunt
    unsigned long cntfrq;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    asm volatile ("msr cntp_tval_el0, %0" : : "r" (cntfrq * second));
    //enable timer interrupt
    asm volatile ("mov x0, 2");
    asm volatile ("ldr x1, =0x40000040");
    asm volatile ("str w0, [x1]");
}


void disable_core_timer() {
    // disable timer
    unsigned long ctl = 0;
    asm volatile ("msr cntp_ctl_el0, %0" : : "r" (ctl));

    // mask timer interrupt
    asm volatile ("mov x0, 0");
    asm volatile ("ldr x1, =0x40000040");
    asm volatile ("str w0, [x1]");
}
```

### timer_handler
1. check which timers to expire
2. callback and clear the timer by set expires to 0
3. disable core timer
4. check if there is another timer awaiting
```
void setTimeout_callback(char* data) {
    // Convert data back to the appropriate type and print the message
    uart_puts(data);
    uart_puts("\n");
    uart_send('\r');
    uart_puts("# ");
}

void timer_handler() {
    asm volatile("msr DAIFSet, 0xf");
    unsigned long cur_time = get_current_time();
    unsigned long next = 9999;
    for(int i=0;i<MAX_TIMER;i++){
        if(timers[i].expires == cur_time){
            uart_puts("\n[TIMER] ");
            uart_int(cur_time);
            uart_puts(" : ");
            timers[i].callback(timers[i].data);
            timers[i].expires = 0;
        }
        else if(timers[i].expires > cur_time){
            if(next > timers[i].expires)
                next = timers[i].expires;
        }
    }
    disable_core_timer();
    if(next != 9999){
        set_timer_interrupt(next - cur_time);
        //uart_puts("resetted another timer\n");
    }
    asm volatile("msr DAIFClr, 0xf");
}
```
## Task Queue
### Modification
uart interrupt: disable write interrupt to avoid a lot of writing task

### Enqueue
Place tasks into the queue during handling with priority (the smaller the higher)
```
void interrupt_handler_entry(){
    int core0_irq = *CORE0_INTERRUPT_SOURCE;
    int iir = *AUX_MU_IIR;
    if (core0_irq & 2){
        create_task(timer_handler, 3);
        // timer_handler();
    }
    else{
        if ((iir & 0x06) == 0x04){
            create_task(uart_read_handler,1);
        }
    }
    execute_task();
}
```
Place write task after read (a char once, so need two tasks for newline)
```
void uart_write_handler(){
    char ch;
    if(write_cur < write_idx){
        *AUX_MU_IO=uart_write_buffer[write_cur]; // i think need interrupt here so cannot asm
        ch = uart_write_buffer[write_cur];
        write_cur++;
    }
    if(ch == '\r'){
        shell(async_cmd);
        write_cur = 0;
        write_idx = 0;
    }
}

void uart_read_handler() {
    char ch = (char)(*AUX_MU_IO);
    if(ch == '\r'){ //command
        //uart_send('\n');
        uart_read_buffer[read_idx] = '\0';
        read_idx = 0;
        strcpy(uart_read_buffer, async_cmd);
        //uart_puts(uart_read_buffer);
        uart_write_buffer[write_idx] = '\n';
        write_idx++;
        create_task(uart_write_handler,2);
        uart_write_buffer[write_idx] = '\r';
        write_idx++;
        //shell(uart_read_buffer);
        //uart_read_buffer[read_idx] = '\0';
    }
    else{
        uart_read_buffer[read_idx] = ch;
        uart_write_buffer[write_idx] = ch; 
        //uart_send(uart_read_buffer[read_idx]);
        read_idx++;
        write_idx++;
    }
    create_task(uart_write_handler,2);
}
```

### Queue
* Enqueue: allocate to a spare place and check the priorty
* Execute: Start executing with the lowest priority (FIFO), then update lowest priority and execute next
```
#define MAX_TASKS 1024 //if boom will boom

char in_buffer[1024];
char exec_buffer[1024];
int in_idx;
int exec_idx;

typedef void (*task_func_t)(void);

typedef struct {
    task_func_t func;
    //void* data;
    int priority; // Optional for prioritized execution
} task_t;

typedef struct {
    task_t tasks[MAX_TASKS];
    int min_priority;
    int task_count;
} task_queue_t;

task_queue_t task_queue;

void create_task(task_func_t callback, unsigned int priority){
    //asm volatile("msr DAIFSet, 0xf");
    //uart_puts("task created!\n");
    int i;
    in_buffer[in_idx] = priority + '0';
    in_idx++;
    for(i=0; i<MAX_TASKS;i++){
        if(task_queue.tasks[i].priority == 0){
            task_queue.task_count++;
            task_queue.tasks[i].priority = priority;
            task_queue.tasks[i].func = callback;
            if(priority < task_queue.min_priority || task_queue.min_priority == 0)
                task_queue.min_priority = priority;
            // uart_puts("current min priority: ");
            // uart_int(task_queue.min_priority);
            break;
        }
    }
    //asm volatile("msr DAIFClr, 0xf");
}

void execute_task(){
    while(task_queue.task_count != 0){
        int next_min = 999;
        for(int i=0; i<MAX_TASKS;i++){
            if(task_queue.tasks[i].priority < task_queue.min_priority && task_queue.tasks[i].priority != 0){
                uart_puts("this should be replaced");
            }
            else if(task_queue.tasks[i].priority == task_queue.min_priority){
                asm volatile("msr DAIFSet, 0xf");
                exec_buffer[exec_idx] = task_queue.tasks[i].priority + '0';
                exec_idx++;
                task_queue.tasks[i].func();
                task_queue.tasks[i].priority = 0;
                task_queue.task_count--;
                asm volatile("msr DAIFClr, 0xf");
            }
            else if(task_queue.tasks[i].priority != 0){
                if(task_queue.tasks[i].priority < next_min)
                    next_min = task_queue.tasks[i].priority;
            }
        }
        if(next_min!=999)
            task_queue.min_priority = next_min;
        else
            task_queue.min_priority = 0;
    }
    exec_buffer[exec_idx] = ' ';
    exec_idx++;
}
```