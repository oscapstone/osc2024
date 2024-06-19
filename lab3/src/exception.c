#include "exception.h"
#include "mini_uart.h"
#include "utils.h"
#include "timer.h"
#include "tasklist.h"
#include "peripherals/irq.h"

extern timer_t *timer_list;

void exception_entry(void) {
    uart_send_string("Warning: exception occurs.\r\n");

    //read spsr_el1
	unsigned long long spsr_el1 = 0;
	asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1)); // "=r" means output operand using general registers, will write to the variable spsr_el1
	uart_send_string("spsr_el1: ");
	uart_send_string_int2hex(spsr_el1);
	uart_send_string("\r\n");

	//read elr_el1
	unsigned long long elr_el1 = 0;
	asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
	uart_send_string("elr_el1: ");
	uart_send_string_int2hex(elr_el1);
	uart_send_string("\r\n");
	
	//esr_el1
	unsigned long long esr_el1 = 0;
	asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));
    uart_send_string("esr_el1: ");
	uart_send_string_int2hex(esr_el1);
	uart_send_string("\r\n");
}

void enable_interrupt(void) 
{
    asm volatile ("msr daifclr, 0xf"); // umask all DAIF -> set to zero
}

void disable_interrupt(void)
{
    asm volatile ("msr daifset, 0xf"); // mask all DAIF -> set to one
}

void irq_exception_handler_c(void)
{
    // uart_send_string("IRQ Exception Occurs!\n");
    unsigned int irq = get32(IRQ_PENDING_1);
    unsigned int interrupt_source = get32(CORE0_INTERRUPT_SOURCE);

    if((irq & IRQ_PENDING_1_AUX_INT) && (interrupt_source & INTERRUPT_SOURCE_GPU))
    {
        // uart_send_string("UART interrupt\r\n");
        uart_irq_handler();
    } 
    else if(interrupt_source & INTERRUPT_SOURCE_CNTPNSIRQ) 
    {
        // uart_send_string("\nTimer interrupt\r\n");
        put32(CORE0_TIMER_IRQ_CTRL, 0);
        create_task(irq_timer_exception, 0);
        execute_tasks_preemptive();
    }
}

void irq_timer_exception(void)
{
    // enable timer iterrupt
    put32(CORE0_TIMER_IRQ_CTRL, 2);

    // disable timer interrupt before entering critical section
    // asm volatile("msr cntp_ctl_el0,%0"::"r"(0));

    while(timer_list)
    {
        unsigned long long current_time;
        asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
        if(timer_list -> expired_time <= current_time) 
        {
            timer_t *timer = timer_list;
            timer -> callback(timer -> data, timer -> timeout);
            timer_list = timer_list -> next;
            if(timer_list)
                timer_list -> prev = 0;
        }
        else
        {
            break;
        }

        if(timer_list)
        {
            asm volatile("msr cntp_cval_el0, %0"::"r"(timer_list -> expired_time));
            asm volatile("msr cntp_ctl_el0, %0"::"r"(1));
        } 
        else 
        {
            asm volatile("msr cntp_ctl_el0, %0"::"r"(0));
        }
    }
}