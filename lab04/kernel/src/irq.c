#include "irq.h"
#include "io.h"
#include "timer.h"
#include "mini_uart.h"
#include "alloc.h"

struct task_struct{
    int priority;
    int dirty;
    struct task_struct *next;
    struct task_struct *prev;
    void (*func_handler)(void);
};

static struct task_struct *task_head = 0;

void task_head_init()
{
    struct task_struct* tmp = 0;
    task_head = tmp;
}

void add_task(void (*func)(void), int priority)
{
    struct task_struct* new_task = (struct task_struct*)simple_malloc(sizeof(struct task_struct));

    new_task->func_handler = func;
    new_task->priority = priority;
    new_task->dirty = 0;
    new_task->next = 0;
    new_task->prev = 0;

    if(!task_head)
    {
        task_head = new_task;
    }
    else
    {
        struct task_struct* curr = task_head;
        while(curr->next && curr->priority <= priority)
        {
            curr = curr->next;
        }
        if(!curr->next && curr->priority <= priority)
        {
            curr->next = new_task;
            new_task->prev = curr;
        }
        else
        {
            new_task->next = curr;
            new_task->prev = curr->prev;
            if(!new_task->prev)
                task_head = new_task;
            curr->prev = new_task;
        }
    }

}

void run_task()
{

    while(task_head && !task_head->dirty)
    {
        disable_irq();
        struct task_struct* tmp = task_head;
        tmp->dirty = 1;
        enable_irq();
        tmp->func_handler();
        disable_irq();
        if(tmp->prev) tmp->prev->next = tmp->next;
        if(tmp->next) tmp->next->prev = tmp->prev;
        if(task_head == tmp) task_head = tmp->next;
        enable_irq();
    }
}


// ==============================================================

static char *entry_error_messages[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1T",		

	"SYNC_INVALID_EL1h",		
	"IRQ_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		

	"SYNC_INVALID_EL0_64",		
	"IRQ_INVALID_EL0_64",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	

	"SYNC_INVALID_EL0_32",		
	"IRQ_INVALID_EL0_32",		
	"FIQ_INVALID_EL0_32",		
	"ERROR_INVALID_EL0_32"	
};

static void show_debug_msg(int type, unsigned long spsr, unsigned long elr, unsigned long esr)
{
    printf("\r\n");
    printf(entry_error_messages[type]);
    printf(", SPSR: ");
    printf_hex(spsr);
    printf(", ELR: ");
    printf_hex(elr);
    printf(", ESR: ");
    printf_hex(esr);
}

static void gpu_irq_handler()
{
    if(*IRQ_PENDING_1 & AUX_INT)
    {

        switch(*AUX_MU_IIR_REG & 0x6)
        {
            case AUX_MU_IIR_REG_READ:
                disable_uart_recv_interrupt();
                add_task(async_uart_read_handler, EL1_IRQ_UART_PRIORITY);
                break;
            case AUX_MU_IIR_REG_WRITE:
                disable_uart_trans_interrupt();
                add_task(async_uart_write_handler, EL1_IRQ_UART_PRIORITY);
                break;
        }
        
    }
}

static void timer_handler()
{
    timer_update_handler();
}


void el1_irq_entry(int type, unsigned long spsr, unsigned long elr, unsigned long esr)
{
    disable_irq();

    unsigned int irq = *CORE0_IRQ_SOURCE;
    switch(irq)
    {
        case SYSTEM_TIMER_IRQ_1:
            core_timer_disable();
            add_task(timer_handler, EL1_IRQ_TIMER_PRIORITY);
            break;
        case GPU_IRQ:
            gpu_irq_handler();
            break;
        default:
            printf("\r\nUnknown irq: ");
            printf_hex(irq);
            break;
    }
    enable_irq();

    run_task();
}

void default_exception_entry(int type, unsigned long spsr, unsigned long elr, unsigned long esr)
{
    
    show_debug_msg(type, spsr, elr, esr);
    while(1);
}

void el0_irq_entry(int type, unsigned long spsr, unsigned long elr, unsigned long esr)
{
    disable_irq();
    show_debug_msg(type, spsr, elr, esr);
    enable_irq();
}

void svc_exception_entry(int type, unsigned long spsr, unsigned long elr, unsigned long esr)
{
    disable_irq();
    show_debug_msg(type, spsr, elr, esr);
    enable_irq();
}
