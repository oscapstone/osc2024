#include "bcm2837/rpi_irq.h"
#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "syscall.h"
#include "signal.h"
#include "sched.h"
#include "memory.h"
#include "irqtask.h"
#include "mmu.h"
#include "debug.h"

// 讀取ESR_EL1暫存器的值
static inline unsigned long read_esr_el1(void)
{
    unsigned long value;
    asm volatile("mrs %0, esr_el1" : "=r"(value));
    return value;
}

// 判斷異常是否由EL0觸發的syscall
static inline int is_el0_syscall()
{
    unsigned long esr_el1 = read_esr_el1();
    unsigned long ec = (esr_el1 >> ESR_EL1_EC_SHIFT) & ESR_EL1_EC_MASK;
    if (ec == ESR_EL1_EC_SVC64)
    {
        return 1;
    }
    return 0;
}

// 定義異常類型名稱
const char *exception_type[] = {
    "Unknown reason",
    "Trapped WFI or WFE instruction execution",
    "Trapped MCR or MRC access with (coproc==0b1111) (AArch32)",
    "Trapped MCRR or MRRC access with (coproc==0b1111) (AArch32)",
    "Trapped MCR or MRC access with (coproc==0b1110) (AArch32)",
    "Trapped LDC or STC access (AArch32)",
    "Trapped FP access",
    "Trapped VMRS access",
    "Trapped PSTATE (AArch32)",
    "Instruction Abort from a lower Exception level",
    "Instruction Abort taken without a change in Exception level",
    "PC alignment fault",
    "Data Abort from a lower Exception level",
    "Data Abort taken without a change in Exception level",
    "SP alignment fault",
    "Trapped floating-point exception",
    "SError interrupt",
    "Breakpoint from a lower Exception level",
    "Breakpoint taken without a change in Exception level",
    "Software Step from a lower Exception level",
    "Software Step taken without a change in Exception level",
    "Watchpoint from a lower Exception level",
    "Watchpoint taken without a change in Exception level",
    "BKPT instruction execution (AArch32)",
    "Vector Catch exception (AArch32)",
    "BRK instruction execution (AArch64)"};

extern list_head_t *run_queue;

// DAIF, Interrupt Mask Bits
void el1_interrupt_enable()
{
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable()
{
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}

unsigned long long int lock_counter = 0;

void lock()
{
    el1_interrupt_disable();
    lock_counter++;
}

void unlock()
{
    // uart_sendlinek("This is unlock\n");
    lock_counter--;
    if (lock_counter < 0)
    {
        while (1)
            ;
    }
    else if (lock_counter == 0)
    {
        el1_interrupt_enable();
    }
}

void el1h_sync_router(trapframe_t *tpf)
{
    uart_sendlinek("\n");
    uart_sendlinek("spsr_el1 : %x\n ", tpf->spsr_el1);
    uart_sendlinek("elr_el1 : %x\n ", tpf->elr_el1);
    uart_sendlinek("sp_el0 : %x\n ", tpf->sp_el0);
    // dump_vma();
    while (1)
        ;
}

void el1h_irq_router(trapframe_t *tpf)
{
    lock();
    // decouple the handler into irqtask queue
    // (1) https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf - Pg.113
    // (2) https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf - Pg.16
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
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
        core_timer_enable();

        if (list_size(run_queue) > 1)
        {
            // uart_sendlinek("el1h_irq_router\n");
            schedule();
        }
    }
    else
    {
        unlock();
        uart_sendlinek("Hello World el1 64 router other interrupt!\r\n");
    }

    // only do signal handler when return to user mode
    if ((tpf->spsr_el1 & 0b1100) == 0)
    {
        check_signal(tpf);
    }
}

void el0_sync_router(trapframe_t *tpf)
{
    static int count = 0;
    unsigned long long esr_el1 = read_esr_el1();

    // esr_el1: Holds syndrome information for an exception taken to EL1.
    esr_el1_t *esr = (esr_el1_t *)&esr_el1;
    if (esr->ec == MEMFAIL_DATA_ABORT_LOWER || esr->ec == MEMFAIL_INST_ABORT_LOWER)
    {
        mmu_memfail_abort_handle(esr);
        return;
    }

    if (!is_el0_syscall())
    {
        const char *exception_name = get_exception_name(esr_el1);
        if (count == 0)
            ERROR("el0_sync_router: exception occurred - %s\r\n", exception_name);
        count++;
        return;
    }

    el1_interrupt_enable(); // Allow UART input during exception
    unsigned long long syscall_no = tpf->x8;
    if (syscall_no > 18)
    {
        uart_sendlinek("SYSCALL No: %d\n", syscall_no);
    }
    // uart_sendlinek("tpf->x8 : %d\n",tpf->x8);
    switch (syscall_no)
    {
        SYSCALL(0, getpid(tpf))
        SYSCALL(1, uartread(tpf, (char *)tpf->x0, tpf->x1))
        SYSCALL(2, uartwrite(tpf, (char *)tpf->x0, tpf->x1))
        SYSCALL(3, exec(tpf, (char *)tpf->x0, (char **)tpf->x1))
        SYSCALL(4, fork(tpf))
        SYSCALL(5, exit(tpf, tpf->x0))
        SYSCALL(6, syscall_mbox_call(tpf, (unsigned char)tpf->x0, (unsigned int *)tpf->x1))
        SYSCALL(7, kill(tpf, (int)tpf->x0))
        SYSCALL(8, signal_register(tpf->x0, (void (*)())tpf->x1))
        SYSCALL(9, signal_kill(tpf->x0, tpf->x1))
        SYSCALL(10, mmap(tpf, (void *)tpf->x0, tpf->x1, tpf->x2, tpf->x3, tpf->x4, tpf->x5);)
        SYSCALL(11, open(tpf, (char *)tpf->x0, tpf->x1);)
        SYSCALL(12, close(tpf, tpf->x0);)
        SYSCALL(13, write(tpf, tpf->x0, (char *)tpf->x1, tpf->x2);)
        SYSCALL(14, read(tpf, tpf->x0, (char *)tpf->x1, tpf->x2);)
        SYSCALL(15, mkdir(tpf, (char *)tpf->x0, tpf->x1);)
        SYSCALL(16, mount(tpf, (char *)tpf->x0, (char *)tpf->x1, (char *)tpf->x2, tpf->x3, (void *)tpf->x4);)
        SYSCALL(17, chdir(tpf, (char *)tpf->x0);)
        SYSCALL(18, lseek64(tpf, tpf->x0, tpf->x1, tpf->x2);)
        SYSCALL(19, ioctl(tpf, tpf->x0, tpf->x1, (void *)tpf->x2);)
        SYSCALL(50, sigreturn(tpf))
        SYSCALL(114, syscall_lock(tpf))
        SYSCALL(514, syscall_unlock(tpf))
    default:
        uart_sendlinek("el0_sync_router other syscall, syscall_no: %d\r\n", syscall_no);
        break;
    }
}

void el0_irq_router(trapframe_t *tpf)
{
    lock();
    // decouple the handler into irqtask queue
    // (1) https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf - Pg.113
    // (2) https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf - Pg.16
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
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
        core_timer_enable();

        if (list_size(run_queue) > 1)
        {
            // uart_sendlinek("el0_irq_router\n");
            schedule();
        }
    }
    else
    {
        unlock();
        uart_sendlinek("Hello World el0 64 router other interrupt!\r\n");
    }

    // only do signal handler when return to user mode
    if ((tpf->spsr_el1 & 0b1100) == 0)
    {
        check_signal(tpf);
    }
}

void invalid_exception_router(unsigned long long x0)
{
    uart_sendlinek("\n invalid exception  %x \n", x0);
    while (1)
        ;
}

// 獲取異常類型名稱
const char *get_exception_name(unsigned long esr_el1)
{
    unsigned long ec = (esr_el1 >> ESR_EL1_EC_SHIFT) & ESR_EL1_EC_MASK;
    if (ec < sizeof(exception_type) / sizeof(exception_type[0]))
    {
        return exception_type[ec];
    }
    return "Unknown exception";
}

// ------------------------------------------------------------------------------------------

/*
Preemption
Now, any interrupt handler can preempt the task’s execution, but the newly enqueued task still needs to wait for the currently running task’s completion.
It’d be better if the newly enqueued task with a higher priority can preempt the currently running task.
To achieve the preemption, the kernel can check the last executing task’s priority before returning to the previous interrupt handler.
If there are higher priority tasks, execute the highest priority task.
*/

int curr_task_priority = 9999; // Small number has higher priority

struct list_head *task_list;
void irqtask_list_init()
{
    task_list = allocator(sizeof(list_head_t));
    INIT_LIST_HEAD(task_list);
}

void irqtask_add(void *task_function, unsigned long long priority)
{
    irqtask_t *the_task = kmalloc(sizeof(irqtask_t)); // free by irq_tasl_run_preemptive()

    // store all the related information into irqtask node
    // manually copy the device's buffer
    the_task->priority = priority;
    the_task->task_function = task_function;
    INIT_LIST_HEAD(&(the_task->listhead));

    // add the timer_event into timer_event_list (sorted)
    // if the priorities are the same -> FIFO
    struct list_head *curr;

    // mask the device's interrupt line
    // el1_interrupt_disable();
    // enqueue the processing task to the event queue with sorting.
    list_for_each(curr, task_list)
    {
        if (((irqtask_t *)curr)->priority > the_task->priority)
        {
            list_add(&(the_task->listhead), curr->prev);
            break;
        }
    }
    // if the priority is lowest
    if (list_is_head(curr, task_list))
    {
        list_add_tail(&(the_task->listhead), task_list);
    }
    // unmask the interrupt line
    // el1_interrupt_enable();
}

void irqtask_run_preemptive()
{
    // el1_interrupt_enable();
    while (!list_empty(task_list))
    {
        // critical section protects new coming node

        lock();
        irqtask_t *the_task = (irqtask_t *)task_list->next;
        // Run new task (early return) if its priority is lower than the scheduled task.
        if (curr_task_priority <= the_task->priority)
        {
            unlock();
            break;
        }
        // get the scheduled task and run it.
        list_del_entry((struct list_head *)the_task);
        int prev_task_priority = curr_task_priority;
        curr_task_priority = the_task->priority;
        // uart_sendlinek("preemptive curr_task_priority: %d\r\n", curr_task_priority);
        unlock();

        irqtask_run(the_task);

        lock();
        curr_task_priority = prev_task_priority;
        unlock();
        // free(the_task);
    }
}

void irqtask_run(irqtask_t *the_task)
{
    ((void (*)())the_task->task_function)();
    kfree(the_task);
}
