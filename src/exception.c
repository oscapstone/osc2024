#include "uart.h"
#include "exception.h"
#include "initrd.h"
#include "shell.h"
#include "string.h"
#include "timer.h"
#include "sched.h"
#include "syscall.h"
#include "queue.h"
#include "interrupt.h"
#include "delays.h"
#include "demo.h"

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
    /* dump registers */
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
    while (1) {
        asm volatile("nop");
    }
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
        exc_handler(esr, elr, spsr, far);
        return;
    }
    switch (svc_num) {
        case 0: 
            //uart_puts("svc 0: system call\n");
            syscall_handler(trapframe);
            break;
        case 1:
            uart_puts("svc 1: user program\nname :");
            shell_input(cmd);
            initrd_usr_prog(cmd);
            break;
        case 2:
            uart_puts("svc 2: enable timer\n");
            core_timer_enable();
            printf("Current time: %d\n", get_current_time());
            break;
        case 3:
            unsigned long timeout;
            char message[64];
            uart_puts("svc 3: set timeout\ntimeout: ");
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
        case 5:
            uart_puts("svc 5: Demo bottom half irq\n");
            demo_bh_irq();
            break;
        default:
            uart_puts("Unknown svc, print reg information\n");
            exc_handler(esr, elr, spsr, far);
            break;
    }
}

/* In UART interrupt handler, I put receiver interrupt to bottom half*/
void uart_interrupt_handler()
{
    char c;

    /* Disable the uart interrupt */
    AUX->AUX_MU_IER_REG &= ~(1 << 0);
    
    if (AUX->AUX_MU_IIR_REG & (0b01 << 1)) { // Transmit holds register empty
        /* Disable the transmit interrupt */
        AUX->AUX_MU_IER_REG &= ~(1 << 1);

        while (!is_empty(&write_buffer)) { // Check the write_buffer status, if it is not empty, then output it.
            c = dequeue_char(&write_buffer);
            if (AUX->AUX_MU_LSR_REG & 0x20) // If the transmitter FIFO can accept at least one byte
                AUX->AUX_MU_IO_REG = c;
        }
    } else if (AUX->AUX_MU_IIR_REG & (0b10 << 1)) { // Receiver holds valid bytes
        if (AUX->AUX_MU_LSR_REG & 0x1) { // Receiver FIFO holds valid bytes
            c = (char) (AUX->AUX_MU_IO_REG); // If we take char from AUX_MU_IO, the interrupt will be cleared.
            c = (c == '\r') ? '\n' : c;

#ifdef ENABLE_TASKLET
            tl_pool[UART_TASKLET - 1].data = (unsigned long) c;
            tasklet_add(&tl_pool[UART_TASKLET - 1]);
            enable_interrupt();
            do_tasklet();
#else
            enqueue_char(&read_buffer, c);
#endif
        }
    } else
        uart_puts("Unknown uart interrupt\n");
    
    /* Enable the uart interrupt */
    AUX->AUX_MU_IER_REG |= (1 << 0);
}

/* IRQ handler */
void irq_router(unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    if (*CORE0_IRQ_SOURCE & (1 << 1)) {
        core_timer_handler();
    } else if (IRQ->IRQ_PENDING1 & (1 << 29)) { // AUX interrupt, from ARM peripherals interrupts table
        uart_interrupt_handler();
    }
}

void invalid_exception_handler(void)
{
    uart_puts("Invalid exception\n");
    while (1) {
        asm volatile("nop");
    }
}

/* Substract the current task counter value and call schedule(). */
void do_timer()
{
    if (--current->counter > 0)
        return;
    schedule();
}

/* Set the next core timer interrupt. Update timer related structure. */
void core_timer_handler()
{
#ifdef ENABLE_TASKLET
    /* Mask the core timer interrupt first. Mask CORE0_TIMER_IRQ_CTRL. Spec says it will be done automatically? */
    *CORE0_TIMER_IRQ_CTRL &= ~(1 << 1);
#endif
    /*     Setup next timer interrupt */
    asm volatile(
        "mrs x0, cntfrq_el0     \n\t"
        "lsr x0, x0, #5         \n\t"
        "msr cntp_tval_el0, x0  \n\t");

#ifdef ENABLE_TASKLET
    tasklet_add(&tl_pool[TIMER_TASKLET - 1]);
    enable_interrupt();
    do_tasklet();

    /* Enable the core timer interrupt */
    *CORE0_TIMER_IRQ_CTRL |= (1 << 1);
#else
    timer_update();
#endif

#ifdef DEBUG_MULTITASKING
    printf("[core_timer_handler] Core timer interrupt\n");
#endif

    do_timer(); // do_timer() may context switch, so we put it after enable the timer interrupt.
}

/* Print the current exception level. */
void print_current_el(void)
{
    unsigned long current_el;
    asm volatile("mrs %0, CurrentEL" : "=r" (current_el));
    printf("-- CurrentEL: %d\n", (current_el >> 2) & 0x3);
}