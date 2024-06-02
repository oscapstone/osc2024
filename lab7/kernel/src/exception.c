#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "irqtask.h"
#include "syscall.h"
#include "sched.h"
#include "signal.h"
#include "mmu.h"
#include "colourful.h"

extern int finish_init_thread_sched;
// extern int syscall_num;
// extern SYSCALL_TABLE_T *syscall_table;
extern char *syscall_table;

void sync_64_router(trapframe_t* tpf)
{
    unsigned long long esr_el1;
    __asm__ __volatile__("mrs %0, esr_el1\n\t": "=r"(esr_el1));

    // esr_el1: Holds syndrome information for an exception taken to EL1.
    esr_el1_t *esr = (esr_el1_t *)&esr_el1;
    if (esr->ec == MEMFAIL_DATA_ABORT_LOWER || esr->ec == MEMFAIL_INST_ABORT_LOWER)
    {
        mmu_memfail_abort_handle(esr);
        return;
    }
    el1_interrupt_enable();


    // check whether it is a syscall 
    if (esr->ec == ESR_ELx_EC_SVC64){
        unsigned long long syscall_no = tpf->x8;
        if (syscall_no > SYSCALL_TABLE_SIZE || syscall_no < 0 )
        {
            // invalid syscall number
            uart_sendline("Invalid syscall number: %d\r\n", syscall_no);
            tpf->x0 = -1;
            return;
        }
        if (((void **) syscall_table)[syscall_no] == 0)
        {
            uart_sendline("Unregisted syscall number: %d\r\n", syscall_no);
            tpf->x0 = -1;
            return;
        }
        ((int (*)(trapframe_t *))(((&syscall_table)[syscall_no])))(tpf);
    }
}

void irq_router(trapframe_t* tpf)
{
    lock();
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IER_REG & 2)
        {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & 1)
        {
            *AUX_MU_IER_REG &= ~(1); // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive();
        }
    }
    else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ)  //from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
        core_timer_enable();

        // check whether init_thread_sched is finished
        if (finish_init_thread_sched == 1)
            schedule();
    }
    else
    {
        uart_puts("Hello World el1 64 router other interrupt!\r\n");
    }
    //only do signal handler when return to user mode
    if ((tpf->spsr_el1 & 0b1100) == 0)
    {
        check_signal(tpf);
        
    }
}

void invalid_exception_router(unsigned long long x0){
    // TBD
}

static unsigned long long lock_count = 0;
void lock()
{
    el1_interrupt_disable();
    lock_count++;
}

void unlock()
{
    lock_count--;
    if (lock_count == 0)
        el1_interrupt_enable();
}
