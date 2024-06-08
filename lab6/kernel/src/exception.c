#include "bcm2837/rpi_irq.h"
#include "bcm2837/rpi_uart1.h"
#include "uart1.h"
#include "exception.h"
#include "timer.h"
#include "memory.h"
#include "stdio.h"
#include "stdint.h"
#include "syscall.h"
#include "sched.h"

extern int8_t need_to_schedule;
extern thread_t *curr_thread;

// DAIF, Interrupt Mask Bits
void el1_interrupt_enable()
{
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable()
{
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}

static uint64_t lock_counter = 0;

void __lock_interrupt()
{
    el1_interrupt_disable();
    lock_counter++;
    // DEBUG("kernel_lock_interrupt counter: %d\r\n", lock_counter);
}

void __unlock_interrupt()
{
    lock_counter--;
    // DEBUG("kernel_unlock_interrupt counter: %d\r\n", lock_counter);
    if (lock_counter < 0)
    {
        ERROR("kernel_lock_interrupt counter error");
        while (1)
            ;
    }
    else if (lock_counter == 0)
    {
        // DEBUG("unlock\r\n");
        el1_interrupt_enable();
    }
}

void el1h_irq_router(trapframe_t *tpf)
{
    kernel_lock_interrupt();
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IER_REG & 2)
        {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            kernel_unlock_interrupt();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & 1)
        {
            *AUX_MU_IER_REG &= ~(1); // Re-enable core timer interrupt when entering core_timer_handler or add_timer
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            kernel_unlock_interrupt();
            irqtask_run_preemptive();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable(); // enable core timer interrupt when entering the handler
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        kernel_unlock_interrupt();
        irqtask_run_preemptive();
    }
    else
    {
        uart_puts("Hello World el1 64 router other interrupt!\r\n");
    }
    if (need_to_schedule == 1)
    {
        need_to_schedule = 0;
        schedule();
    }
    // only do signal handler when return to user mode
    if ((tpf->spsr_el1 & 0b1100) == 0)
    {
        run_pending_signal(tpf);
    }
}

#define ESR_EL1_EC_SHIFT 26
#define ESR_EL1_EC_MASK 0x3F
#define ESR_EL1_EC_SVC64 0x15

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

// https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-
const char *IFSC_table[] = {
    "Address size fault, level 0 of translation or translation table base register.",                          // 0b000000
    "Address size fault, level 1",                                                                             // 0b000001
    "Address size fault, level 2",                                                                             // 0b000010
    "Address size fault, level 3",                                                                             // 0b000011
    "Translation fault, level 0",                                                                              // 0b000100
    "Translation fault, level 1",                                                                              // 0b000101
    "Translation fault, level 2",                                                                              // 0b000110
    "Translation fault, level 3",                                                                              // 0b000111
    "",                                                                                                        // 0b001000
    "Access flag fault, level 1",                                                                              // 0b001001
    "Access flag fault, level 2",                                                                              // 0b001010
    "Access flag fault, level 3",                                                                              // 0b001011
    "Permission fault, level 0",                                                                               // 0b001100
    "Permission fault, level 1",                                                                               // 0b001101
    "Permission fault, level 2",                                                                               // 0b001110
    "Permission fault, level 3",                                                                               // 0b001111
    "Synchronous External abort, not on translation table walk or hardware update of translation table.",      // 0b010000
    "Synchronous External abort on translation table walk or hardware update of translation table, level -2.", // 0b010001
    ""                                                                                                         // 0b010010
    "Synchronous External abort on translation table walk or hardware update of translation table, level -1.", // 0b010011
    "Synchronous External abort on translation table walk or hardware update of translation table, level 0.",  // 0b010100
    "Synchronous External abort on translation table walk or hardware update of translation table, level 1.",  // 0b010101
    "Synchronous External abort on translation table walk or hardware update of translation table, level 2.",  // 0b010110
    "Synchronous External abort on translation table walk or hardware update of translation table, level 3.",  // 0b010111
    "Synchronous parity or ECC error on memory access, not on translation table walk."                         // 0b011000
};

// 讀取ESR_EL1暫存器的值
static inline uint64_t read_esr_el1(void)
{
    uint64_t value;
    asm volatile("mrs %0, esr_el1" : "=r"(value));
    return value;
}

// 讀取ELR_EL1暫存器的值
static inline uint64_t read_elr_el1(void)
{
    uint64_t value;
    asm volatile("mrs %0, elr_el1" : "=r"(value));
    return value;
}

// 讀取SPSR_EL1暫存器的值
static inline uint64_t read_spsr_el1(void)
{
    uint64_t value;
    asm volatile("mrs %0, spsr_el1" : "=r"(value));
    return value;
}

// 判斷異常是否由EL0觸發的syscall
static inline int is_el0_syscall(void)
{
    uint64_t esr_el1 = read_esr_el1();
    uint64_t ec = (esr_el1 >> ESR_EL1_EC_SHIFT) & ESR_EL1_EC_MASK;
    if (ec == ESR_EL1_EC_SVC64)
    {
        return 1;
    }
    return 0;
}

// 獲取異常類型名稱
const char *get_exception_name(uint64_t esr_el1)
{
    uint64_t ec = (esr_el1 >> ESR_EL1_EC_SHIFT) & ESR_EL1_EC_MASK;
    if (ec < sizeof(exception_type) / sizeof(exception_type[0]))
    {
        return exception_type[ec];
    }
    return "Unknown exception";
}

void el0_sync_router(trapframe_t *tpf)
{
    static int count = 0;
    uint64_t esr_el1_value = read_esr_el1();
    uint64_t elr_el1_value = read_elr_el1();
    uint64_t spsr_el1_value = read_spsr_el1();
    // esr_el1: Holds syndrome information for an exception taken to EL1.
    esr_el1_t *esr = (esr_el1_t *)&esr_el1_value;
    if (esr->ec == MEMFAIL_DATA_ABORT_LOWER || esr->ec == MEMFAIL_INST_ABORT_LOWER)
    {
        mmu_memfail_abort_handle(esr);
        DEBUG("mmu_memfail_abort_handle sp_el0: 0x%x\n", tpf->sp_el0);
        return;
    }
    else if (!is_el0_syscall())
    {
        const char *exception_name = get_exception_name(esr_el1_value);
        if (count == 0)
        {
            ERROR("el0_sync_router: exception occurred - %s\r\n", exception_name);
            ERROR("spsr_el1: 0x%016lx\r\n", spsr_el1_value);
            ERROR("elr_el1: 0x%016lx\r\n", elr_el1_value);
            ERROR("esr_el1: 0x%016lx\r\n", esr_el1_value);
        }
        count++;
        return;
    }

    uint64_t syscall_no = tpf->x8 >= MAX_SYSCALL ? MAX_SYSCALL : tpf->x8;
    // DEBUG("syscall_no: %d\r\n", syscall_no);

    // only work with GCC
    void *syscall_router[] = {&&__getpid_label,           // 0
                              &&__uart_read_label,        // 1
                              &&__uart_write_label,       // 2
                              &&__exec_label,             // 3
                              &&__fork_label,             // 4
                              &&__exit_label,             // 5
                              &&__mbox_call_label,        // 6
                              &&__kill_label,             // 7
                              &&__signal_register_label,  // 8
                              &&__signal_kill_label,      // 9
                              &&__mmap_label,             // 10
                              &&__signal_return_label,    // 11
                              &&__lock_interrupt_label,   // 12
                              &&__unlock_interrupt_label, // 13
                              &&__invalid_syscall_label}; // 14

    goto *syscall_router[syscall_no];

__getpid_label:
    DEBUG("sys_getpid\r\n");
    // dump tpf
    DEBUG("tpf->x0: %d\r\n", tpf->x0);
    DEBUG("tpf->x1: %d\r\n", tpf->x1);
    DEBUG("tpf->x2: %d\r\n", tpf->x2);
    DEBUG("tpf->x3: %d\r\n", tpf->x3);
    DEBUG("tpf->x4: %d\r\n", tpf->x4);
    DEBUG("tpf->x5: %d\r\n", tpf->x5);
    DEBUG("tpf->x6: %d\r\n", tpf->x6);
    DEBUG("tpf->x7: %d\r\n", tpf->x7);
    DEBUG("tpf->x8: %d\r\n", tpf->x8);
    DEBUG("tpf->x9: %d\r\n", tpf->x9);
    DEBUG("tpf->x10: %d\r\n", tpf->x10);
    DEBUG("tpf->x11: %d\r\n", tpf->x11);
    DEBUG("tpf->x12: %d\r\n", tpf->x12);
    DEBUG("tpf->x13: %d\r\n", tpf->x13);
    DEBUG("tpf->x14: %d\r\n", tpf->x14);
    DEBUG("tpf->x15: %d\r\n", tpf->x15);
    DEBUG("tpf->x16: %d\r\n", tpf->x16);
    DEBUG("tpf->x17: %d\r\n", tpf->x17);
    DEBUG("tpf->x18: %d\r\n", tpf->x18);
    DEBUG("tpf->x19: %d\r\n", tpf->x19);
    DEBUG("tpf->x20: %d\r\n", tpf->x20);
    DEBUG("tpf->x21: %d\r\n", tpf->x21);
    DEBUG("tpf->x22: %d\r\n", tpf->x22);
    DEBUG("tpf->x23: %d\r\n", tpf->x23);
    DEBUG("tpf->x24: %d\r\n", tpf->x24);
    DEBUG("tpf->x25: %d\r\n", tpf->x25);
    DEBUG("tpf->x26: %d\r\n", tpf->x26);
    DEBUG("tpf->x27: %d\r\n", tpf->x27);
    DEBUG("tpf->x28: %d\r\n", tpf->x28);
    DEBUG("tpf->x29: %d\r\n", tpf->x29);
    DEBUG("tpf->x30: %d\r\n", tpf->x30);
    DEBUG("tpf->sp_el0: 0x%x\r\n", tpf->sp_el0);
    DEBUG("tpf->spsr_el1: 0x%x\r\n", tpf->spsr_el1);
    DEBUG("tpf->elr_el1: 0x%x\r\n", tpf->elr_el1);
    tpf->x0 = sys_getpid(tpf);
    return;

__uart_read_label:
    // DEBUG("sys_uart_read\r\n");
    tpf->x0 = sys_uart_read(tpf, (char *)tpf->x0, tpf->x1);
    return;

__uart_write_label:
    // DEBUG("sys_uart_write\r\n");
    tpf->x0 = sys_uart_write(tpf, (char *)tpf->x0, (char **)tpf->x1);
    // DEBUG("writer size: %d\r\n", tpf->x0);
    return;

__exec_label:
    DEBUG("sys_exec\r\n");
    tpf->x0 = sys_exec(tpf, (char *)tpf->x0, (char **)tpf->x1);
    return;

__fork_label:
    DEBUG("sys_fork\r\n");
    tpf->x0 = sys_fork(tpf);
    return;

__exit_label:
    DEBUG("sys_exit\r\n");
    tpf->x0 = sys_exit(tpf, tpf->x0);
    return;

__mbox_call_label:
    DEBUG("sys_mbox_call\r\n");
    tpf->x0 = sys_mbox_call(tpf, (uint8_t)tpf->x0, (unsigned int *)tpf->x1);
    DEBUG("mbox_call return: %d\r\n", tpf->x0);
    return;

__kill_label:
    DEBUG("sys_kill\r\n");
    tpf->x0 = sys_kill(tpf, (int)tpf->x0);
    return;

__signal_register_label:
    DEBUG("sys_signal_register\r\n");
    tpf->x0 = sys_signal_register(tpf, (int)tpf->x0, (void (*)(void))(tpf->x1));
    return;

__signal_kill_label:
    DEBUG("sys_signal_kill\r\n");
    tpf->x0 = sys_signal_kill(tpf, (int)tpf->x0, (int)tpf->x1);
    return;

__mmap_label:
    DEBUG("sys_mmap\r\n");
    tpf->x0 = sys_mmap(tpf, (void *)tpf->x0, (size_t)tpf->x1, (int)tpf->x2, (int)tpf->x3, (int)tpf->x4, (int)tpf->x5);
    return;

__signal_return_label:
    DEBUG("sys_signal_return\r\n");
    sys_signal_return(tpf);
    return;

__lock_interrupt_label:
    DEBUG("sys_lock_interrupt\r\n");
    sys_lock_interrupt(tpf);
    return;

__unlock_interrupt_label:
    DEBUG("sys_unlock_interrupt\r\n");
    sys_unlock_interrupt(tpf);
    return;

__invalid_syscall_label:
    ERROR("Invalid system call number: %d\r\n", syscall_no);
    return;
}

void el0_irq_64_router(trapframe_t *tpf)
{
    kernel_lock_interrupt();
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IER_REG & 2)
        {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            kernel_unlock_interrupt();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & 1)
        {
            *AUX_MU_IER_REG &= ~(1); // Re-enable core timer interrupt when entering core_timer_handler or add_timer
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            kernel_unlock_interrupt();
            irqtask_run_preemptive();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable(); // enable core timer interrupt when entering the handler
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        kernel_unlock_interrupt();
        irqtask_run_preemptive();
    }
    else
    {
        uart_puts("Hello world! el0_irq_64_router other interrupt!\r\n");
    }
    if (need_to_schedule == 1)
    {
        need_to_schedule = 0;
        schedule();
    }
    // only do signal handler when return to user mode
    if ((tpf->spsr_el1 & 0b1100) == 0)
    {
        run_pending_signal(tpf);
    }
}

void invalid_exception_router(uint64_t x0)
{
    static int count = 0;
    if (count == 0)
    {
        ERROR("invalid exception router: %d, pid: %d\r\n", x0, curr_thread->pid);
        count++;
    }
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
    task_list = kmalloc(sizeof(irqtask_t));
    INIT_LIST_HEAD(task_list);
}

void irqtask_add(void *task_function, uint64_t priority)
{
    // if (task_function == uart_r_irq_handler)
    //     DEBUG("irqtask_add uart_r_irq_handler, kmalloc\r\n");
    // else if (task_function == uart_w_irq_handler)
    //     DEBUG("irqtask_add uart_w_irq_handler, kmalloc\r\n");
    // else if (task_function == core_timer_handler)
    //     DEBUG("irqtask_add core_timer_handler, kmalloc\r\n");
    irqtask_t *the_task = kmalloc(sizeof(irqtask_t)); // free by irq_tasl_run_preemptive()
    // store all the related information into irqtask node
    // manually copy the device's buffer
    the_task->priority = priority;
    the_task->task_function = task_function;
    INIT_LIST_HEAD(&(the_task->listhead));

    // add the timer_event into timer_event_list (sorted)
    // if the priorities are the same -> FIFO
    struct list_head *curr;

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
}

void irqtask_run_preemptive()
{
    while (!list_empty(task_list))
    {
        kernel_lock_interrupt();
        irqtask_t *the_task = (irqtask_t *)task_list->next;
        // struct list_head *curr;
        // uart_puts("---------------------- list for each ----------------------\r\n");
        // list_for_each(curr, task_list)
        // {
        //     if (((irqtask_t *)curr)->task_function == uart_r_irq_handler)
        //         uart_puts("irqtask_run_preemptive uart_r_irq_handler\r\n");
        //     else if (((irqtask_t *)curr)->task_function == uart_w_irq_handler)
        //         uart_puts("irqtask_run_preemptive uart_w_irq_handler\r\n");
        //     else if (((irqtask_t *)curr)->task_function == core_timer_handler)
        //         uart_puts("irqtask_run_preemptive core_timer_handler\r\n");
        // }
        // uart_puts("--------------------------- end ---------------------------\r\n");
        // Run new task (early return) if its priority is lower than the scheduled task.
        if (curr_task_priority <= the_task->priority)
        {
            // DEBUG("irqtask_run_preemptive early return\r\n");
            kernel_unlock_interrupt();
            break;
        }
        list_del_entry((struct list_head *)the_task);
        int prev_task_priority = curr_task_priority;
        curr_task_priority = the_task->priority;

        // DEBUG("irqtask_run\r\n");
        kernel_unlock_interrupt();
        irqtask_run(the_task);

        kernel_lock_interrupt();

        curr_task_priority = prev_task_priority;
        // DEBUG("irqtask_run_preemptive kfree\r\n");
        kfree(the_task);
        kernel_unlock_interrupt();
    }
}

void irqtask_run(irqtask_t *the_task)
{
    ((void (*)())the_task->task_function)();
}
