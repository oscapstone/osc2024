#include "mini_uart.h"
#include "exception.h"
#include "shell.h"
#include "timer.h"

extern char *__boot_loader_addr;
extern unsigned long long __code_size;
extern unsigned long long __begin;
extern void set_for_el_switch(void);
char *_dtb;
char *exceptionLevel;
static int EL2_to_EL1_flag = 1;

// x0 is for the parameter
void main(char *arg)
{

    exceptionLevel = arg;
    // put_int(exceptionLevel);
    // uart_puts("\n");
    // el1_interrupt_enable();
    // timer_init(); //這會跳去EL0
    
    uart_init();
    uart_flush_FIFO();
    irqtask_list_init();
    timer_list_init();

    uart_interrupt_enable();
    //core_timer_enable();
    el1_interrupt_enable();  // enable interrupt in EL1 -> EL1

    shell_banner();


    while (1)
    {
        shell();
    }
}
void code_relocate(char *addr)
{
    unsigned long long size = (unsigned long long)&__code_size;
    char *start = (char *)&__begin;
    for (unsigned long long i = 0; i < size; i++)
    {
        addr[i] = start[i];
    }

    ((void (*)(char *))addr)(_dtb);
}