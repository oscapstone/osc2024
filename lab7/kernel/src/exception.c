#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "irqtask.h"
#include "syscall.h"
#include "sched.h"
#include "signal.h"
#include "mmu.h"

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
    unsigned long long syscall_no = tpf->x8;
    if (syscall_no == 0)       { getpid(tpf);                                                                 }
    else if(syscall_no == 1)   { uartread(tpf,(char *) tpf->x0, tpf->x1);                                     }
    else if (syscall_no == 2)  { uartwrite(tpf,(char *) tpf->x0, tpf->x1);                                    }
    else if (syscall_no == 3)  { exec(tpf,(char *) tpf->x0, (char **)tpf->x1);                                }
    else if (syscall_no == 4)  { fork(tpf);                                                                   }
    else if (syscall_no == 5)  { exit(tpf,tpf->x0);                                                           }
    else if (syscall_no == 6)  { syscall_mbox_call(tpf,(unsigned char)tpf->x0, (unsigned int *)tpf->x1);      }
    else if (syscall_no == 7)  { kill(tpf, (int)tpf->x0);                                                     }
    else if (syscall_no == 8)  { signal_register(tpf->x0, (void (*)())tpf->x1);                               }
    else if (syscall_no == 9)  { signal_kill(tpf->x0, tpf->x1);                                               }
    else if (syscall_no == 10) { mmap(tpf,(void *)tpf->x0,tpf->x1,tpf->x2,tpf->x3,tpf->x4,tpf->x5);           }
    else if (syscall_no == 11) { open(tpf, (char*)tpf->x0, tpf->x1);                                                     }
    else if (syscall_no == 12) { close(tpf, tpf->x0);                                                                    }
    else if (syscall_no == 13) { write(tpf, tpf->x0, (char *)tpf->x1, tpf->x2);                                          }
    else if (syscall_no == 14) { read(tpf, tpf->x0, (char *)tpf->x1, tpf->x2);                                           }
    else if (syscall_no == 15) { mkdir(tpf, (char *)tpf->x0, tpf->x1);                                                   }
    else if (syscall_no == 16) { mount(tpf, (char *)tpf->x0, (char *)tpf->x1, (char *)tpf->x2, tpf->x3, (void*)tpf->x4); }
    else if (syscall_no == 17) { chdir(tpf, (char *)tpf->x0);                                                            }
    else if (syscall_no == 18) { lseek64(tpf, tpf->x0, tpf->x1, tpf->x2);                                                }
    else if (syscall_no == 19) { ioctl(tpf, tpf->x0, tpf->x1, (void*)tpf->x2);                                           }
    else if (syscall_no == 50) { sigreturn(tpf);                                                                         }
    el1_interrupt_disable();
}

void irq_router(trapframe_t* tpf)
{
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) {
        if (*AUX_MU_IIR_REG & (1 << 1)) // can write
        {
            *AUX_MU_IER_REG &= ~(2);  // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive();
        }
        else if (*AUX_MU_IIR_REG & (0b10 << 1)) // can read
        {
            *AUX_MU_IER_REG &= ~(1);  // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            irqtask_run_preemptive();
        }
    } else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        irqtask_run_preemptive();
        core_timer_enable();
        el1_interrupt_disable();
        if (run_queue->next->next != run_queue) schedule();
    }
    if ((tpf->spsr_el1 & 0b1100) == 0) { check_signal(tpf); }
    el1_interrupt_disable();
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
