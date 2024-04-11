//#include "printf.h"
#include "timer.h"
#include "uart.h"
#include "tasklist.h"

#define CORE0_IRQ_SOURCE ((volatile unsigned int *)0x40000060)  //p.16 BCM2836-peri
#define IRQ_PEND_1 ((volatile unsigned int *)0x3f00B204)    //p.112 BCM2837
#define IRQS1 (volatile unsigned int *)0x3f00b210   //p.112 BCM2837

extern struct queue uart_write, uart_read;

void svc_router(unsigned long spsr, unsigned long elr, unsigned long esr)
{
    unsigned int svc_n = esr & 0xFFFFFF;

    switch (svc_n)
    {
    case 0:
        uart_puts("\nspsr_el1\t");
        uart_hex(spsr);
        uart_puts("\nelr_el1\t\t");
        uart_hex(elr);
        uart_puts("\nesr_el1\t\t");
        uart_hex(esr);
        uart_puts("\n");

        break;

    case 4:
        asm volatile(
            "ldr x0, =0x345             \n\t"
            "msr spsr_el1, x0           \n\t"
            "ldr x0, = shell_start      \n\t"
            "msr elr_el1,x0             \n\t"
            "eret                       \n\t");

    default:
        break;
    }

    return;
}

void print_invalid_entry_message()
{
    uart_puts("invalid exception!\n");
    return;
}
//https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/DAIF--Interrupt-Mask-Bits
//disable DAIF
void disable_irq()
{
    asm volatile("msr DAIFSet, 0xf");
}
//enalble DAIF
void enable_irq()
{
    asm volatile("msr DAIFClr, 0xf");
}

void mini_uart_interrupt_enable()
{
    *IRQS1 |= (1 << 29);    //p.113 p.116 BCM2837 Enable IRQs 1 => Set to enable IRQ source 31:0 p.8 aux
    *AUX_MU_IER = 0x1;  //enable interrupt
    queue_init(&uart_read, 1024);
    //queue_init(&uart_write, 1024);
}

void irq_router()
{
    unsigned int irq = *CORE0_IRQ_SOURCE;   //the source bits are for the IRQ/FIQ.
    unsigned int irq1 = *IRQ_PEND_1;    //IRQ pending source 31:0
    disable_irq();
    //CNTPNSIRQ[0-3] Non-secure physical timer event.   https://developer.arm.com/documentation/100964/1100-00/Processor-Components/Cortex-A-processor-components/ARMCortexA15xnCT-component
    //irq[1]=CNTPNSIRQ interrupt  irq[0]=CNTPSIRQ interrupt (Physical Timer -1) p.16 BCM2836
    if (irq & 0x2)
    {
        timer_router();
        //uart_puts("timer exception\n");
        /*task_t *current = task_head;
        uart_puts("task in list:\n");
        while (current)
        {
            uart_puts(current->name);
            uart_puts("\n");
            current = current->next;
        }*/
        //execute_tasks();
    }
    //p.113 BCM2837 
    else if (irq1 & (1 << 29))
    {
        mini_uart_handler();
    }
    enable_irq();
    //execute_tasks();
}
