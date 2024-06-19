#include "peripherals/rpi_irq.h"
#include "peripherals/mini_uart.h"
#include "exception.h"
#include "mini_uart.h"
#include "utils.h"
#include "timer.h"
#include "memory.h"
#include "sched.h"
#include "syscall.h"

int                 curr_task_priority = 9999; // Small number has higher priority
struct              list_head *irqtask_list;
extern list_head_t  *run_queue;
extern int          done_sched_init;
extern int          need_to_schedule;

#define ESR_EL1_EC_SHIFT 26
#define ESR_EL1_EC_MASK 0x3F
#define ESR_EL1_EC_SVC64 0x15

static unsigned long long lock_count = 0;

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
    "BRK instruction execution (AArch64)"
};

void print_currentEL(){
    unsigned long long currentEL;
    __asm__ __volatile__("mrs %0, currentEL\n\t" : "=r"(currentEL));
    
    switch (currentEL) {
        case 0:
            uart_puts("Current EL: EL0\r\n");
            break;
        case 4:
            uart_puts("Current EL: EL1\r\n");
            break;
        case 8:
            uart_puts("Current EL: EL2\r\n");
            break;
        case 12:
            uart_puts("Current EL: EL3\r\n");
            break;
        default:
            uart_puts("Current EL: UNKNOWN\r\n");
            break;
    }
}

void print_lock() {
    uart_puts("[*] Locks: %d\r\n", lock_count);
}

// DAIF, Interrupt Mask Bits
void el1_interrupt_enable() {
    __asm__ __volatile__("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable() {
    __asm__ __volatile__("msr daifset, 0xf"); // mask all DAIF
}

uint64_t read_esr_el1(void) {
    uint64_t value;
    asm volatile("mrs %0, esr_el1" : "=r"(value));
    return value;
}

uint64_t read_elr_el1(void) {
    uint64_t value;
    asm volatile("mrs %0, elr_el1" : "=r"(value));
    return value;
}

uint64_t read_spsr_el1(void) {
    uint64_t value;
    asm volatile("mrs %0, spsr_el1" : "=r"(value));
    return value;
}

const char *get_exception_name(uint64_t esr_el1) {
    uint64_t ec = (esr_el1 >> ESR_EL1_EC_SHIFT) & ESR_EL1_EC_MASK;
    if (ec < sizeof(exception_type) / sizeof(exception_type[0]))
    {
        return exception_type[ec];
    }
    return "Unknown exception";
}

int is_el0_syscall(void) {
    uint64_t esr_el1 = read_esr_el1();
    // uart_puts("esr_el1: 0x%x\r\n", esr_el1);
    uint64_t ec = (esr_el1 >> ESR_EL1_EC_SHIFT) & ESR_EL1_EC_MASK;
    if (ec == ESR_EL1_EC_SVC64) {
        return 1;
    }
    return 0;
}

void el1h_irq_router(trapframe_t* tpf) {
    lock();
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) {
        if (*AUX_MU_IER_REG & 2) {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_write_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        } else if (*AUX_MU_IER_REG & 1) {
            *AUX_MU_IER_REG &= ~(1);
            irqtask_add(uart_read_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive();
        }
    } else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) {
        core_timer_disable();
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
        core_timer_enable();
    } else {
        uart_puts("UNKNOWN el1h_irq_router\r\n");
    }

    if (need_to_schedule == 1) {
        need_to_schedule = 0;
        schedule();
    }
    // only do signal handler when return to user mode
    if ((tpf->spsr_el1 & 0b1100) == 0) {
        run_pending_signal();
    }
}

void el0_sync_router(trapframe_t *tpf) {
    static int count = 0;
    uint64_t esr_el1 = read_esr_el1();
    if (!is_el0_syscall()) {
        const char *exception_name = get_exception_name(esr_el1);
        if (count == 0)
            uart_puts("el0_sync_router: exception occurred - %s\r\n", exception_name);
        count++;
        return;
    }
     // Basic #3 - Based on System Call Format in Video Player’s Test Program
    uint64_t syscall_no = tpf->x8 >= MAX_SYSCALL ? MAX_SYSCALL : tpf->x8;
    // //"syscall_no: %d\r\n", syscall_no);
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
    tpf->x0 = sys_getpid(tpf);
    return;

__uart_read_label:
    tpf->x0 = sys_uart_read(tpf, (char *)tpf->x0, tpf->x1);
    return;

__uart_write_label:
    tpf->x0 = sys_uart_write(tpf, (char *)tpf->x0, (char **)tpf->x1);
    return;

__exec_label:
    tpf->x0 = sys_exec(tpf, (char *)tpf->x0, (char **)tpf->x1);
    return;

__fork_label:
    tpf->x0 = sys_fork(tpf);
    return;

__exit_label:
    tpf->x0 = sys_exit(tpf, tpf->x0);
    return;

__mbox_call_label:
    tpf->x0 = sys_mbox_call(tpf, (uint8_t)tpf->x0, (unsigned int *)tpf->x1);
    return;

__kill_label:
    tpf->x0 = sys_kill(tpf, (int)tpf->x0);
    return;

__signal_register_label:
    tpf->x0 = sys_signal_register(tpf, (int)tpf->x0, (void (*)(void))(tpf->x1));
    return;

__signal_kill_label:
    tpf->x0 = sys_signal_kill(tpf, (int)tpf->x0, (int)tpf->x1);
    return;

__signal_return_label:
    sys_signal_return(tpf);
    return;

__lock_interrupt_label:
    sys_lock_interrupt(tpf);
    return;

__unlock_interrupt_label:
    sys_unlock_interrupt(tpf);
    return;

__invalid_syscall_label:
    uart_puts("Invalid system call number: %d\r\n", syscall_no);
    return;
}

void el0_irq_64_router(trapframe_t *tpf) {
    lock();
    // decouple the handler into irqtask queue
    // (1) https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf - Pg.113
    // (2) https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf - Pg.16
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        if (*AUX_MU_IER_REG & 2) {
            *AUX_MU_IER_REG &= ~(2); // disable write interrupt
            irqtask_add(uart_write_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive(); // run the queued task before returning to the program.
        }
        else if (*AUX_MU_IER_REG & 1) {
            *AUX_MU_IER_REG &= ~(1); // Re-enable core timer interrupt when entering core_timer_handler or add_timer
            irqtask_add(uart_read_irq_handler, UART_IRQ_PRIORITY);
            unlock();
            irqtask_run_preemptive();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ) // from CNTPNS (core_timer) // A1 - setTimeout run in el1
    {
        core_timer_disable(); // enable core timer interrupt when entering the handler
        irqtask_add(core_timer_handler, TIMER_IRQ_PRIORITY);
        unlock();
        irqtask_run_preemptive();
    }
    else
    {
        uart_puts("Hello world! el0_irq_64_router other interrupt!\r\n");
    }
    if (need_to_schedule == 1) {
        need_to_schedule = 0;
        schedule();
    }
    // only do signal handler when return to user mode
    if ((tpf->spsr_el1 & 0b1100) == 0) {
        run_pending_signal();
    }
}

void invalid_exception_router(uint64_t x0) {
    uart_puts("Invalid exception : 0x%x\r\n",x0);
}

void lock() {
    el1_interrupt_disable();
    lock_count++;
}

void unlock() {
    lock_count--;
    if (lock_count == 0)
        el1_interrupt_enable();
    else if (lock_count < 0) {
        uart_puts("lock interrupt counter error\r\n");
        while(1);
    }
}

void irqtask_list_init() {
    irqtask_list = malloc(sizeof(irqtask_t));
    INIT_LIST_HEAD(irqtask_list);
}

void irqtask_add(void *task_function, unsigned long long priority) {
    irqtask_t *irq_task = malloc(sizeof(irqtask_t));
    irq_task->priority = priority;
    irq_task->task_function = task_function;
    INIT_LIST_HEAD(&(irq_task->listhead));

    struct list_head *ptr;
    list_for_each(ptr, irqtask_list) {
        if (((irqtask_t *)ptr)->priority > irq_task->priority) {
            list_add(&(irq_task->listhead), ptr->prev);
            break;
        }
    }
    // Lowest Priority
    if (list_is_head(ptr, irqtask_list)) {
        list_add_tail(&(irq_task->listhead), irqtask_list);
    }
}

void irqtask_run(irqtask_t *task) {
    ((void (*)())task->task_function)();
}

void irqtask_run_preemptive() {
    while (!list_empty(irqtask_list)) {
        lock();
        irqtask_t *irq_task = (irqtask_t *)irqtask_list->next;

        // uart_puts("Current task prio: ");
        // put_int(curr_task_priority);
        // uart_puts("\r\n");

        if (curr_task_priority <= irq_task->priority) {
            // uart_puts("unlock\r\n");
            unlock();
            break;
        }
        
        // struct list_head* curr;
        // uart_puts("DEBUG: ");
        // list_for_each(curr, irqtask_list) {
        //     put_int(((irqtask_t *)curr)->priority);
        //     uart_puts(" ");
        // }
        // uart_puts("\r\n");

        list_del_entry((struct list_head *)irq_task);
        int prev_task_priority = curr_task_priority;
        curr_task_priority = irq_task->priority;
        
        unlock();
        irqtask_run(irq_task);
        lock();

        curr_task_priority = prev_task_priority;
        unlock();
    }
}
