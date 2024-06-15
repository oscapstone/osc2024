#include "mini_uart.h"
#include "exception.h"
#include "shell.h"
#include "timer.h"
#include "memory.h"
#include "sched.h"

extern char *__boot_loader_addr;
extern unsigned long long __code_size;
extern unsigned long long __begin;
extern void set_for_el_switch(void);
char *_dtb;
char *exceptionLevel;
static int EL2_to_EL1_flag = 1;

extern thread_t *curr_thread;
extern thread_t *threads[];
// x0 is for the parameter
void main(char *arg)
{
    exceptionLevel = arg;

    uart_init();
    uart_interrupt_enable();
    uart_flush_FIFO();
    el1_interrupt_disable(); // enable interrupt in EL1 -> EL1
    init_memory_space();
    irqtask_list_init();
    timer_list_init();

    el1_interrupt_enable(); // enable interrupt in EL1 -> EL1
    shell_banner();

    uart_hex(curr_thread);
    uart_puts("\r\n");
    init_thread_sched();
    schedule_timer();
    timer_init();
    lock();
    uart_puts("=====curr thread======= : ");
    uart_puts(curr_thread->name);
    uart_puts("\r\n");
    set_current_thread_context(&curr_thread->context);
    load_context(&curr_thread->context); // jump to idle thread and unlock interrupt
}