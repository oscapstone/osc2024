#include "uart.h"
#include "exception.h"
#include "initrd.h"
#include "shell.h"
#include "string.h"
#include "timer.h"
#include "sched.h"
#include "syscall.h"
#include "queue.h"


extern void from_el1_to_el0(void);
extern void core_timer_enable(void);

/**
 * common exception handler
 */
void exc_handler(unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
    switch(esr >> 26) {
        case 0b000000: uart_puts("Unknown"); break;
        case 0b000001: uart_puts("Trapped WFI/WFE"); break;
        case 0b001110: uart_puts("Illegal execution"); break;
        case 0b010101: uart_puts("System call"); break;
        case 0b100000: uart_puts("Instruction abort, lower EL"); break;
        case 0b100001: uart_puts("Instruction abort, same EL"); break;
        case 0b100010: uart_puts("Instruction alignment fault"); break;
        case 0b100100: uart_puts("Data abort, lower EL"); break;
        case 0b100101: uart_puts("Data abort, same EL"); break;
        case 0b100110: uart_puts("Stack alignment fault"); break;
        case 0b101100: uart_puts("Floating point"); break;
        default: uart_puts("Unknown"); break;
    }
    switch(esr&0x3) {
        case 0: uart_puts(" at level 0"); break;
        case 1: uart_puts(" at level 1"); break;
        case 2: uart_puts(" at level 2"); break;
        case 3: uart_puts(" at level 3"); break;
    }
    // decode data abort cause
    if(esr >> 26 == 0b100100 || esr >> 26 == 0b100101) {
        uart_puts(", ");
        switch((esr >> 2) & 0x3) {
            case 0: uart_puts("Address size fault"); break;
            case 1: uart_puts("Translation fault"); break;
            case 2: uart_puts("Access flag fault"); break;
            case 3: uart_puts("Permission fault"); break;
        }
        switch(esr & 0x3) {
            case 0: uart_puts(" at level 0"); break;
            case 1: uart_puts(" at level 1"); break;
            case 2: uart_puts(" at level 2"); break;
            case 3: uart_puts(" at level 3"); break;
        }
    }
    // dump registers
    uart_puts(":\n  ESR_EL1 ");
    uart_hex(esr>>32);
    uart_hex(esr);
    uart_puts(" ELR_EL1 ");
    uart_hex(elr>>32);
    uart_hex(elr);
    uart_puts("\n SPSR_EL1 ");
    uart_hex(spsr>>32);
    uart_hex(spsr);
    uart_puts(" FAR_EL1 ");
    uart_hex(far>>32);
    uart_hex(far);
    uart_puts("\n");

    // no return from exception for now
    // while(1);
}

/**
 * System call handler. Take system call number from trap frame (x8 register).
 * Then use system call number to call the function in system call table.
*/
void syscall_handler(struct trapframe *trapframe)
{
    int syscall_num = trapframe->x[8];

    if (syscall_num >= 0 && syscall_num < SYSCALL_NUM) {
        if (sys_call_table[syscall_num](trapframe)) {
            uart_puts("System call failed\n");
        }
    } else {
        uart_puts("Unknown syscall\n");
    }
    return;
}

/**
 * svc handler
 */
void svc_handler(unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far, struct trapframe *trapframe)
{
    int svc_num = esr & 0xffff;
    char cmd[64];

    if (esr >> 26 != 0b010101) {
        uart_puts("Not a svc call\n");
        exc_handler(esr, elr, spsr, far);
        return;
    }
    switch (svc_num) {
        case 0: 
            syscall_handler(trapframe);
            break;
        case 1:
            uart_puts("svc 1: user program\n");
            uart_puts("name :");
            shell_input(cmd);
            initrd_usr_prog(cmd);
            break;
        case 2:
            uart_puts("svc 2: enable timer\n");
            core_timer_enable();
            break;
        case 3:
            unsigned long timeout;
            char message[64];
            uart_puts("svc 3: set timeout\n");
            uart_puts("timeout: ");
            shell_input(cmd);
            timeout = (unsigned long) atoi(cmd);

            uart_puts("\nmessage: ");
            shell_input(message);
            uart_puts("\n");
            timer_set(timeout, message);
            break;
        case 4:
            uart_puts("svc 4: User Program System call\n");
            exc_handler(esr, elr, spsr, far); // for Lab 3, print out the reg info.
            break;
        default:
            uart_puts("Unknown svc, print reg information\n");
            exc_handler(esr, elr, spsr, far);
            break;
    }
}

void uart_interrupt_handler()
{
    if (AUX->AUX_MU_IIR_REG & (0b01 << 1)) { // Transmit holds register empty
        /* Check the write_buffer status, if it is not empty, then output it */
        while (!is_empty(&write_buffer)) {
            char c = dequeue(&write_buffer);
            if (AUX->AUX_MU_LSR_REG & 0x20) {
                AUX->AUX_MU_IO_REG = c;
            } else {
                uart_puts("Transmit FIFO is full\n");
                break;
            }
        }

        /* If write_buffer is empty, we should disable the transmitter interrupt. */
        if (is_empty(&write_buffer)) {
            AUX->AUX_MU_IER_REG &= ~(1 << 1); // disable transmit interrupt
        }
    } else if (AUX->AUX_MU_IIR_REG & (0b10 << 1)) { // Receiver holds valid bytes
        if (AUX->AUX_MU_LSR_REG & 0x1) { // Receiver FIFO holds valid bytes
            char r = (char) (AUX->AUX_MU_IO_REG); // If we take char from AUX_MU_IO, the interrupt will be cleared.
            r = (r == '\r') ? '\n' : r;

            /* ouput the char to screen (uart_send without pooling) */
            // if (AUX->AUX_MU_LSR_REG & 0x20)
            //     AUX->AUX_MU_IO_REG = r;
            
            /* output the char to read buffer. */
            enqueue(&read_buffer, r);
        } else {
            uart_puts("Something unexpected\n");
        }
    } else 
        uart_puts("Something unexpected\n");
}

void irq_router(unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    // uart_puts("IRQ handler\n");
    int irq_core0 = *CORE0_IRQ_SOURCE;
    int irq_pend1 = IRQ->IRQ_PENDING1;
    if (irq_core0 & (1 << 1)) {
        core_timer_handler();
    } else if (irq_pend1 & (1 << 29)) { // AUX interrupt, from ARM peripherals interrupts table
        uart_interrupt_handler();
    }
}

/* Substract the current task counter value */
void do_timer()
{
    // printf("Current task: %d, counter %d\n", current->task_id, current->counter);
    if (--current->counter > 0)
        return;
    schedule();
}

void core_timer_handler()
{
    unsigned int seconds;
    asm volatile(
        "mrs x0, cntpct_el0     \n\t"
        "mrs x1, cntfrq_el0     \n\t"
        "udiv %0, x0, x1        \n\t": "=r" (seconds));
    printf("\nseconds: %d\n", seconds);

    /* Setup next timer interrupt*/
    asm volatile(
        "mrs x0, cntfrq_el0     \n\t"
        "mov x1, #2             \n\t"
        "mul x0, x0, x1         \n\t"
        "msr cntp_tval_el0, x0  \n\t");
    
    // timer_update();
    // do_timer();
}

void print_current_el(void)
{
    unsigned long current_el;
    asm volatile("mrs %0, CurrentEL" : "=r" (current_el));
    uart_puts("==== CurrentEL: ");
    uart_hex((current_el >> 2) & 0x3);
    uart_puts("\n");
}