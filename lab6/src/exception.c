#include "exception.h"
#include "bcm2837/rpi_uart1.h"
#include "irqtask.h"
#include "sched.h"
#include "signal.h"
#include "syscall.h"
#include "timer.h"
#include "uart1.h"

void sync_64_router(trapframe_t *tpf)
{
    el1_interrupt_enable();
    unsigned long long syscall_no = tpf->x8;
    switch (syscall_no) {
    case 0:
        getpid(tpf);
        break;
    case 1:
        uartread(tpf, (char *)tpf->x0, tpf->x1);
        break;
    case 2:
        uartwrite(tpf, (char *)tpf->x0, tpf->x1);
        break;
    case 3:
        exec(tpf, (char *)tpf->x0, (char **)tpf->x1);
        break;
    case 4:
        fork(tpf);
        break;
    case 5:
        exit(tpf, tpf->x0);
        break;
    case 6:
        syscall_mbox_call(tpf, (unsigned char)tpf->x0, (unsigned int *)tpf->x1);
        break;
    case 7:
        kill(tpf, (int)tpf->x0);
        break;
    case 8:
        signal_register(tpf->x0, (void (*)())tpf->x1);
        break;
    case 9:
        signal_kill(tpf->x0, tpf->x1);
        break;
    case 50:
        sigreturn(tpf);
        break;
    default:
        break;
    }
}

void irq_router(trapframe_t *tpf)
{
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) {
        if (*AUX_MU_IIR_REG & (1 << 1)) // can write
        {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive();
        }
        else if (*AUX_MU_IIR_REG & (0b10 << 1)) // can read
        {
            *AUX_MU_IER_REG &= ~(1); // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        irqtask_run_preemptive();
        core_timer_enable();

        if (run_queue->next->next != run_queue)
            schedule();
    }
    if ((tpf->spsr_el1 & 0b1100) == 0) {
        check_signal(tpf);
    }
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
