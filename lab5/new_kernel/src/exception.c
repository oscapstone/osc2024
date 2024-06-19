#include "exception_table.h"
#include "timer.h"
#include "rpi_irq.h"
#include "rpi_mini_uart.h"
#include "rpi_gpio.h"
#include "mini_uart.h"
#include "utility.h"
#include "stdint.h"
#include "exception.h"
#include "memory.h"
#include "syscall.h"

extern int8_t need_to_schedule;
struct list_head *task_list;
unsigned long long int lock_counter = 0;

void lock()
{
    el1_interrupt_disable();
    lock_counter++;
    if(lock_counter!= 1){
        (lock_counter);
    }

}

void unlock()
{
    lock_counter--;
    if (lock_counter < 0)
    {
        uart_puts("lock counter error\r\n");
        while (1)
            ;
    }
    else if (lock_counter == 0)
    {
        el1_interrupt_enable();
    }
}
void irqtask_list_init()
{
    task_list = kmalloc(sizeof(irqtask_t));

    INIT_LIST_HEAD(task_list);
}

void irqtask_add(void *task_function, unsigned long long priority)
{
    irqtask_t *the_task = kmalloc(sizeof(irqtask_t));

    // store all the related information into irqtask node
    // manually copy the device's buffer
    the_task->priority = priority;
    the_task->task_function = task_function;
    INIT_LIST_HEAD(&(the_task->listhead));

    // add the timer_event into timer_event_list (sorted)
    // if the priorities are the same -> FIFO
    struct list_head *curr;

    // mask the device's interrupt line
    el1_interrupt_disable();
    // enqueue the processing task to the event queue with sorting.
    list_for_each(curr, task_list)
    {
        if (((irqtask_t *)curr)->priority > the_task->priority)
        {
            list_add_tail(&(the_task->listhead), task_list);
            break;
        }
    }
    // if the priority is lowest
    if (list_is_head(curr, task_list))
    {
        list_add_tail(&the_task->listhead, task_list);
    }
    // unmask the interrupt line
    // el1_interrupt_enable();
}
int curr_task_priority = 9999; // Small number has higher priority

void irqtask_run_preemptive()
{
    // 倘若還有irq要處理
    while (!list_empty(task_list))
    {
        lock();
        // 給一塊 the_task空間，存放接下來要執行的task
        irqtask_t *the_task = (irqtask_t *)task_list->next;
        // 一塊current list head
        struct list_head *curr;

        // 倘若現階段的task 優先高於 下一個要執行的，break離開，不執行下一段
        if (curr_task_priority <= the_task->priority)
        {
            unlock();
            break;
        }
        // 到此為有task 要搶佔了，將the_task 移除佇列，因為要執行了
        list_del_entry((struct list_head *)the_task);

        // 將被插隊的task 優先留下
        int prev_task_priority = curr_task_priority;
        // 把目前要執行的task priority設為現在要執行的task
        curr_task_priority = the_task->priority;

        unlock();

        // the task搶佔
        irqtask_run(the_task);

        lock();

        // 搶佔的執行完畢 即可還原原本的執行模式
        curr_task_priority = prev_task_priority;
        kfree(the_task); // Adding at 6/13 for lab5
        unlock();
    }
}

void irqtask_run(irqtask_t *the_task)
{
    ((void (*)())the_task->task_function)();
}

void sync_exc_router(unsigned long spsr, unsigned long elr, unsigned long esr)
{
    int ec = (esr >> 26) & 0b111111;
    if (ec == 0b010101)
    { // system call
        uart_puts("[Syscall] \n");
        uart_puts("Saved Program Status (SPSR): 0x");
        uart_hex(spsr);
        uart_puts("\n");

        uart_puts("Exception return address (ELR): 0x");
        uart_hex(elr);
        uart_puts("\n");

        uart_puts("Exception syndrome (ESR): 0x");
        uart_hex(esr);
        uart_puts("\n");
    }
    else
    {
        uart_puts("Not svc syscall but el0 syn router \n");
    }

    if (need_to_schedule == 1)
    {
        need_to_schedule = 0;
        schedule();
    }
    while (1)
    {
        /* code */
    }
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

// 讀取ESR_EL1暫存器的值
static inline uint64_t read_esr_el1(void)
{
    uint64_t value;
    asm volatile("mrs %0, esr_el1" : "=r"(value));
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
    uart_puts("  ------- [+] el0 sync router | ");
    uart_puts("------- lock counter is ");
    put_int(lock_counter);
    uart_puts(" ------- ");

    // static int count = 0;
    // uint64_t esr_el1 = read_esr_el1();
    // if (!is_el0_syscall())
    // {
    //     const char *exception_name = get_exception_name(esr_el1);
    //     if (count == 0)
    //         uart_puts("El0 sync router \r\n");
    //     count++;
    //     return;
    // }


    // Basic #3 - Based on System Call Format in Video Player’s Test Program
    uint64_t syscall_no = tpf->x8 >= MAX_SYSCALL ? MAX_SYSCALL : tpf->x8;
    uart_puts("================= syscall no ");
    put_int(syscall_no);
    uart_puts(" ================= \r\n");
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
                              &&__signal_return_label,    // 10
                              &&__lock_interrupt_label,   // 11
                              &&__unlock_interrupt_label, // 12
                              &&__invalid_syscall_label}; // 13

    goto *syscall_router[syscall_no];

__getpid_label:
    // //"sys_getpid\r\n");
    tpf->x0 = sys_getpid(tpf);
    return;

__uart_read_label:
    tpf->x0 = sys_uart_read(tpf, (char *)tpf->x0, tpf->x1);
    return;

__uart_write_label:
    tpf->x0 = sys_uart_write(tpf, (char *)tpf->x0, (char **)tpf->x1);
    return;

__exec_label:
    // //"sys_exec\r\n");
    tpf->x0 = sys_exec(tpf, (char *)tpf->x0, (char **)tpf->x1);
    return;

__fork_label:
    //"sys_fork\r\n");
    tpf->x0 = sys_fork(tpf);
    return;

__exit_label:
    //"sys_exit\r\n");
    tpf->x0 = sys_exit(tpf, tpf->x0);
    return;

__mbox_call_label:
    //"sys_mbox_call\r\n");
    tpf->x0 = sys_mbox_call(tpf, (uint8_t)tpf->x0, (unsigned int *)tpf->x1);
    //"mbox_call return: %d\r\n", tpf->x0);
    return;

__kill_label:
    //"sys_kill\r\n");
    tpf->x0 = sys_kill(tpf, (int)tpf->x0);
    return;

__signal_register_label:
    //"sys_signal_register\r\n");
    // tpf->x0 = sys_signal_register(tpf, (int)tpf->x0, (void (*)(void))(tpf->x1));
    return;

__signal_kill_label:
    //"sys_signal_kill\r\n");
    // tpf->x0 = sys_signal_kill(tpf, (int)tpf->x0, (int)tpf->x1);
    return;

__signal_return_label:
    //"sys_signal_return\r\n");
    // sys_signal_return(tpf);
    return;

__lock_interrupt_label:
    //"sys_lock_interrupt\r\n");
    // sys_lock_interrupt(tpf);
    return;

__unlock_interrupt_label:
    //"sys_unlock_interrupt\r\n");
    // sys_unlock_interrupt(tpf);
    return;

__invalid_syscall_label:
    // ERROR("Invalid system call number: %d\r\n", syscall_no);
    return;
}

void irq_exc_router()
{
    lock();

    // GPU IRQ 57: UART Interrupt
    // if (irq_basic_pending & (1 << 19)) {
    //     uart_intr_handler();
    // }
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTR_SRC & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IER_REG & 2)
        { // & (0b10) : enable write

            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_w_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & 1)
        {
            // & (0b01) : enable read
            *AUX_MU_IER_REG &= ~(1); // disable read interrupt
            irqtask_add(uart_r_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive();
        }
    }
    else if (*CORE0_INTR_SRC & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        // uart_puts("timer interr\n");
        core_timer_disable();
        irqtask_add(core_timer_hadler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
        core_timer_enable();
    }
    else
    {
        uart_puts("Hello World el 64 router other interrupt!\r\n");
    }
    if (need_to_schedule == 1)
    {
        need_to_schedule = 0;
        schedule();
        // uart_puts("Interrupt Switch \r\n");
    }
}

void not_implemented()
{
    uart_puts("kenel panic because of not implemented function...\n");
    while (1)
        ;
}

void show_exception_status(int type, unsigned long esr, unsigned long address)
{
    uart_puts("not now for Exception status.. \n");
}
