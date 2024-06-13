#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "irqtask.h"
#include "syscall.h"
#include "sched.h"
#include "signal.h"
#include "shell.h"
#include "mmu.h"

void svc_router(trapframe_t *tp)
{
    unsigned long long esr_el1;
    __asm__ __volatile__("mrs %0, esr_el1\n\t" : "=r"(esr_el1));
    esr_el1_t *esr = (esr_el1_t *)&esr_el1;
    if (esr->ec == MEMFAIL_DATA_ABORT_LOWER || esr->ec == MEMFAIL_INST_ABORT_LOWER)
    {
        //uart_sendline("esr->iss: %x\r\n", esr->iss);
        //uart_sendline("esr->ec: %x\r\n", esr->ec);
        mmu_memfail_abort_handle(esr);
        return;
    }

    enable_irq();   // TODO: don't change

    unsigned long long syscall_no = tp->x8;
    if (syscall_no == 0)       { getpid(tp);                                                                 }
    else if(syscall_no == 1)   { uartread(tp,(char *) tp->x0, tp->x1);                                     }
    else if (syscall_no == 2)  { uartwrite(tp,(char *) tp->x0, tp->x1);                                    }
    else if (syscall_no == 3)  { exec(tp,(char *) tp->x0, (char **)tp->x1);                                }
    else if (syscall_no == 4)  { fork(tp);                                                                   }
    else if (syscall_no == 5)  { exit(tp,tp->x0);                                                           }
    else if (syscall_no == 6)  { syscall_mbox_call(tp,(unsigned char)tp->x0, (unsigned int *)tp->x1);      }
    else if (syscall_no == 7)  { kill(tp, (int)tp->x0);                                                     }
    else if (syscall_no == 8)  { register_signal(tp->x0, (void (*)())tp->x1);                               }
    else if (syscall_no == 9)  { signal_kill(tp->x0, tp->x1);                                               }
    else if (syscall_no == 10) { mmap(tp,(void *)tp->x0,tp->x1,tp->x2,tp->x3,tp->x4,tp->x5);           }
    else if (syscall_no == 50) { sigreturn(tp);                                                              }
}

void irq_router(trapframe_t *tp)
{
    // bcm2835 - Pg.113
    // bcm2836 - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IIR_REG & (0b01 << 1))
        {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            add_task_list(uart_w_irq_handler, UART_PRIORITY);
            preemption(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IIR_REG & (0b10 << 1))
        {
            *AUX_MU_IER_REG &= ~(1); // disable read interrupt
            add_task_list(uart_r_irq_handler, UART_PRIORITY);
            preemption();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        add_task_list(core_timer_handler, TIMER_PRIORITY);
        preemption();
        core_timer_enable();

        if (run_queue->next->next != run_queue)
            schedule();
    }

    // spsr[3:2]: EL  spsr[1]: set 0 spsr[0]: using what EL's sp
    // exception from EL0
    if ((tp->spsr_el1 & 0b1100) == 0)
    {
        check_signal(tp);
    }
}

void invalid_exception_router(unsigned long long x0)
{
    uart_sendline("%x: invalid exception\n", x0);
    unsigned long long tmp;
    __asm__ __volatile__("mrs %0, esr_el1" : "=r"(tmp));
    uart_sendline("exception syndrome: %x\n", tmp);
    cmd_el();
    while (1)
    {
    }
}

static unsigned long long lock_count = 0;

void lock()
{
    disable_irq();
    lock_count++;
}

void unlock()
{
    lock_count--;
    if (lock_count == 0)
        enable_irq();
}