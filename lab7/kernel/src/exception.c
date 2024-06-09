#include "gpio.h"
#include "uart.h"
#include "task.h"
#include "timer.h"
#include "schedule.h"
#include "mailbox.h"
#include "allocator.h"
#include "mm.h"
#include "vfs.h"
#include "exception.h"

#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LSR ((volatile unsigned int *)(MMIO_BASE + 0x00215054))

#define TX_PRIORITY 4
#define RX_PRIORITY 4
#define TIMER_PRIORITY 0

extern char read_buf[MAX_SIZE];
extern char write_buf[MAX_SIZE];
extern int read_front;
extern int read_back;
extern int write_front;
extern int write_back;

extern task_heap *task_hp;
extern timer_heap *timer_hp;

void enable_interrupt()
{
    asm volatile("msr DAIFClr, 0xf");
}

void disable_interrupt()
{
    asm volatile("msr DAIFSet, 0xf");
}

void exception_entry()
{
    uart_puts("In Exception handle\n");

    unsigned long long spsr_el1 = 0;
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
    uart_puts("spsr_el1: ");
    uart_hex_lower_case(spsr_el1);
    uart_puts("\n");

    unsigned long long elr_el1 = 0;
    asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
    uart_puts("elr_el1: ");
    uart_hex_lower_case(elr_el1);
    uart_puts("\n");

    unsigned long long esr_el1 = 0;
    asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));
    uart_puts("esr_el1: ");
    uart_hex_lower_case(esr_el1);
    uart_puts("\n");

    return;
}

void el1_sync_handler_entry()
{
    uart_puts("Into sync handler\n");
    unsigned long long esr;
    unsigned long long addr;
    asm volatile(
        "mrs %0, esr_el1\n"
        "mrs %1, far_el1\n"
        : "=r"(esr), "=r"(addr));

    int DFSC = esr & 0b111111;
    uart_puts("DFSC: ");
    uart_hex_lower_case(DFSC);
    uart_puts(", OS can't handle it.\n");

    while (1)
        ;
}

void el0_svc_handler_entry(struct ucontext *trapframe)
{
    int get_syscall_no = trapframe->x[8];
    switch (get_syscall_no)
    {
    case SYS_GET_PID:
        sys_getpid(trapframe);
        break;
    case SYS_UART_READ:
        sys_uartread(trapframe);
        break;
    case SYS_UART_WRITE:
        sys_uartwrite(trapframe);
        break;
    case SYS_EXEC:
        sys_exec(trapframe);
        break;
    case SYS_FORK:
        sys_fork(trapframe);
        break;
    case SYS_EXIT:
        sys_exit(trapframe);
        break;
    case SYS_MBOX_CALL:
        sys_mbox_call(trapframe);
        break;
    case SYS_KILL:
        sys_kill(trapframe);
        break;
    case SYS_SIGNAL:
        sys_signal(trapframe);
        break;
    case SYS_SIGNAL_KILL:
        sys_signal_kill(trapframe);
        break;
    case SYS_MMAP:
        sys_mmap(trapframe);
        break;

    case SYS_OPEN:
        sys_open(trapframe);
        break;
    case SYS_CLOSE:
        sys_close(trapframe);
        break;
    case SYS_WRITE:
        sys_write(trapframe);
        break;
    case SYS_READ:
        sys_read(trapframe);
        break;
    case SYS_MKDIR:
        sys_mkdir(trapframe);
        break;
    case SYS_MOUNT:
        sys_mount(trapframe);
        break;
    case SYS_CHDIR:
        sys_chdir(trapframe);
        break;

    case SYS_SIGRETURN:
        sys_sigreturn(trapframe);
        break;
    default:
        break;
    }
}

void el0_da_handler_entry()
{
    unsigned long long esr;
    unsigned long long addr;
    asm volatile(
        "mrs %0, esr_el1\n"
        "mrs %1, far_el1\n"
        : "=r"(esr), "=r"(addr));

    int DFSC = esr & 0b111111;

    if (DFSC == TRANSLATION_FAULT_L0 || DFSC == TRANSLATION_FAULT_L1 || DFSC == TRANSLATION_FAULT_L2 || DFSC == TRANSLATION_FAULT_L3)
    {
        do_translation_fault(addr);
    }
    else if (DFSC == PERMISSION_FAULT_L0 || DFSC == PERMISSION_FAULT_L1 || DFSC == PERMISSION_FAULT_L2 || DFSC == PERMISSION_FAULT_L3)
    {
        do_permission_fault(addr);
    }
    else
    {
        uart_puts("DFSC: ");
        uart_hex_lower_case(DFSC);
        uart_puts(", v_add: ");
        uart_hex_lower_case(addr);
        uart_puts(", OS can't handle it.\n");
        while (1)
            ;
    }
}

void el0_ia_handler_entry()
{
    unsigned long long esr;
    unsigned long long addr;
    asm volatile(
        "mrs %0, esr_el1\n"
        "mrs %1, far_el1\n"
        : "=r"(esr), "=r"(addr));

    int DFSC = esr & 0b111111;

    if (DFSC == TRANSLATION_FAULT_L0 || DFSC == TRANSLATION_FAULT_L1 || DFSC == TRANSLATION_FAULT_L2 || DFSC == TRANSLATION_FAULT_L3)
    {
        do_translation_fault(addr);
    }
    else if (DFSC == PERMISSION_FAULT_L0 || DFSC == PERMISSION_FAULT_L1 || DFSC == PERMISSION_FAULT_L2 || DFSC == PERMISSION_FAULT_L3)
    {
        do_permission_fault(addr);
    }
    else
    {
        uart_puts("DFSC: ");
        uart_hex_lower_case(DFSC);
        uart_puts(", v_add: ");
        uart_hex_lower_case(addr);
        uart_puts(", OS can't handle it.\n");
        while (1)
            ;
    }
}

void irq_handler_entry()
{
    if (*CORE0_INTERRUPT_SOURCE & (1 << 8)) // interrupt is from GPU
    {
        if (*AUX_MU_IIR & 0b100) // check if it's receiver interrupt.
        {
            disable_uart_rx_interrupt(); // masks the device’s interrupt line
            task t;
            t.callback = rx_task;
            t.priority = RX_PRIORITY;

            disable_interrupt();
            task_heap_insert(task_hp, t);
            enable_interrupt();

            do_task();
        }
        else if (*AUX_MU_IIR & 0b010) // check if it's transmiter interrupt
        {
            disable_uart_tx_interrupt(); // masks the device’s interrupt line
            task t;
            t.callback = tx_task;
            t.priority = TX_PRIORITY;

            disable_interrupt();
            task_heap_insert(task_hp, t);
            enable_interrupt();

            do_task();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & (1 << 1)) // interrupt is from CNTPNSIRQ
    {
        core_timer_disable(); // masks the device’s interrupt line
        task t;
        t.callback = timer_task;
        t.priority = TIMER_PRIORITY;

        disable_interrupt();
        task_heap_insert(task_hp, t);
        enable_interrupt();

        do_task();
    }
}

void rx_task()
{
    if ((read_back + 1) % MAX_SIZE == read_front) // if buffer is full, then return.
        return;

    while ((*AUX_MU_LSR & 1) && ((read_back + 1) % MAX_SIZE != read_front))
    {
        read_buf[read_back] = uart_read();
        read_back = (read_back + 1) % MAX_SIZE;
    }
}

void tx_task()
{
    if (write_front == write_back) // if buffer is empty, then return.
        return;

    while (write_front != write_back)
    {
        uart_write(write_buf[write_front]);
        write_front = (write_front + 1) % MAX_SIZE;
    }
}

void timer_task()
{
    timer t = timer_heap_extractMin(timer_hp);
    if (t.expire == -1) // the timer heap is empty
        return;

    t.callback(t.data, t.executed_time);

    if (timer_hp->size > 0)
    {
        set_min_expire();
        core_timer_enable();
    }
    else
        core_timer_disable();
}

void sys_getpid(struct ucontext *trapframe)
{
    trapframe->x[0] = get_current_task()->id;
}

void sys_uartread(struct ucontext *trapframe)
{
    enable_interrupt(); // to enable nested interrupts in system call, because the interrupt may be closed when go into synchronous handler?

    char *buf = (char *)trapframe->x[0];
    int size = trapframe->x[1];

    for (int i = 0; i < size; i++)
        buf[i] = uart_read();

    buf[size] = '\0';
    trapframe->x[0] = size;

    disable_interrupt();
}

void sys_uartwrite(struct ucontext *trapframe)
{
    char *buf = (char *)trapframe->x[0];
    int size = trapframe->x[1];

    for (int i = 0; i < size; i++)
        uart_write(buf[i]);

    trapframe->x[0] = size;
}

void sys_exec(struct ucontext *trapframe)
{
    const char *name = (const char *)trapframe->x[0];
    char **const argv = (char **const)trapframe->x[1];
    do_exec(name, argv);
    trapframe->x[0] = -1; // if do_exec is falut
}

void sys_fork(struct ucontext *trapframe)
{
    task_struct *parent = get_current_task();
    task_struct *child = task_create(return_from_fork, 0);
    child->priority++;
    kfree(child->ustack);
    child->ustack = NULL;

    int kstack_offset = parent->kstack - (void *)trapframe;
    for (int i = 0; i < kstack_offset; i++) // copy kstack content
        *((char *)child->kstack - i) = *((char *)parent->kstack - i);

    for (int i = 0; i < SIG_NUM; i++) // copy signal handler
    {
        child->is_default_signal_handler[i] = parent->is_default_signal_handler[i];
        child->signal_handler[i] = parent->signal_handler[i];
    }

    unsigned long long code_start, code_size;
    for (struct vm_area_struct *cur = parent->mm_struct->mmap; cur != NULL; cur = cur->vm_next)
    {
        if (cur->vm_type == CODE) // find the code section in VMA of the parent process
        {
            code_start = translate_v_to_p(parent->mm_struct->pgd, cur->vm_start);
            code_size = cur->vm_end - cur->vm_start;
            break;
        }
    }

    child->cpu_context.sp = (unsigned long long)child->kstack - kstack_offset; // revise the right kernel stack pointer
    child->cpu_context.fp = (unsigned long long)child->kstack;

    init_mm_struct(child->mm_struct);
    // map to code
    mappages(child->mm_struct, CODE, 0, code_start, code_size, PROT_READ | PROT_EXEC, MAP_ANONYMOUS);
    // map to ustack
    mappages(child->mm_struct, STACK, 0xffffffffb000, (unsigned long long)(parent->ustack) - 4096 * 4 - VA_START, 4096 * 4, PROT_READ | PROT_WRITE, MAP_ANONYMOUS);
    // map to MMIO_BASE
    mappages(child->mm_struct, IO, 0x3c000000, 0x3c000000, 0x40000000 - 0x3c000000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS);

    change_all_pgd_prot(parent->mm_struct->pgd, PROT_READ);        // change parent prcess pte to read only
    copy_pgd_table(parent->mm_struct->pgd, child->mm_struct->pgd); // child copy parent's page entry

    struct ucontext *child_trapframe = (struct ucontext *)child->cpu_context.sp;

    trapframe->x[0] = child->id; // return child's id
    child_trapframe->x[0] = 0;
}

void sys_exit(struct ucontext *trapframe)
{
    task_exit();
}

void sys_mbox_call(struct ucontext *trapframe)
{
    unsigned char ch = (unsigned char)trapframe->x[0];
    unsigned int *mbox = (unsigned int *)trapframe->x[1];

    int size = mbox[0] / 4;

    for (int i = 0; i < size; i++)
        mailbox[i] = mbox[i];

    trapframe->x[0] = mailbox_call(ch);

    for (int i = 0; i < size; i++)
        mbox[i] = mailbox[i];
}

void sys_kill(struct ucontext *trapframe)
{
    int pid = (int)trapframe->x[0];
    for (task_struct *cur = run_queue.head[0]; cur != NULL; cur = cur->next) // find the task
        if (cur->id == pid)
            cur->state = EXIT;
}

void sys_signal(struct ucontext *trapframe)
{
    int SIGNAL = (int)trapframe->x[0];
    void (*handler)() = (void (*)())trapframe->x[1];

    task_struct *cur = get_current_task();

    cur->is_default_signal_handler[SIGNAL] = 0;
    cur->signal_handler[SIGNAL] = handler;
}

void sys_signal_kill(struct ucontext *trapframe)
{
    int pid = (int)trapframe->x[0];
    int SIGNAL = (int)trapframe->x[1];
    for (task_struct *cur = run_queue.head[0]; cur != NULL; cur = cur->next) // find the task
        if (cur->id == pid)
            cur->received_signal = SIGNAL;
}

void sys_mmap(struct ucontext *trapframe)
{
    task_struct *cur_task = get_current_task();

    unsigned long long addr = (unsigned long long)trapframe->x[0];
    unsigned long long len = (unsigned long long)trapframe->x[1];
    int prot = (int)trapframe->x[2];
    int flags = (int)trapframe->x[3];

    len = ((len >> 12) << 12) + ((len & 0xfff) == 0 ? 0 : 4096); // align len
    unsigned long long p_addr = (unsigned long long)kmalloc(len);
    for (int i = 0; i < len; i++)
        *((char *)(p_addr) + i) = 0;

    if (addr) // if addr is not NULL, then try to allocate it
    {
        int addr_is_free = 1;
        addr = ((addr >> 12) << 12) + ((addr & 0xfff) == 0 ? 0 : 4096); // align len
        for (struct vm_area_struct *vma = cur_task->mm_struct->mmap; vma != NULL; vma = vma->vm_next)
        {
            if ((vma->vm_end > addr && vma->vm_start <= addr) || (vma->vm_end > addr + len && vma->vm_start <= addr + len))
                addr_is_free = 0;
        }

        if (addr_is_free == 0) // addr is overlay to other VMA, try to find the free region by brute-force
        {
            addr = 0x0;
            while (1)
            {
                int overlay = 0;
                for (struct vm_area_struct *vma = cur_task->mm_struct->mmap; vma != NULL; vma = vma->vm_next)
                    if ((vma->vm_end > addr && vma->vm_start <= addr) || (vma->vm_end > addr + len && vma->vm_start <= addr + len))
                        overlay = 1;

                if (overlay == 0)
                    break;
                else
                    addr += 0x1000;
            }
        }
    }
    else // addr is NULL, try to find the free region by brute-force
    {
        addr = 0x0;
        while (1)
        {
            int overlay = 0;
            for (struct vm_area_struct *vma = cur_task->mm_struct->mmap; vma != NULL; vma = vma->vm_next)
                if ((vma->vm_end > addr && vma->vm_start <= addr) || (vma->vm_end > addr + len && vma->vm_start <= addr + len))
                    overlay = 1;

            if (overlay == 0)
                break;
            else
                addr += 0x1000;
        }
    }

    mappages(cur_task->mm_struct, DATA, addr, p_addr - VA_START, len, prot, flags); // map addr
    trapframe->x[0] = addr;
}

void sys_open(struct ucontext *trapframe)
{
    const char *pathname = (const char *)trapframe->x[0];
    int flags = (int)trapframe->x[1];

    /*uart_puts("open: ");
    uart_puts(pathname);
    uart_puts("\n");*/

    struct task_struct* cur_task = get_current_task();
    struct file *file = vfs_open(pathname, flags);

    if (file == NULL) // open file fail
    {
        trapframe->x[0] = -1;
        return;
    }

    for (int i = 0; i < NR_OPEN_DEFAULT; i++)
        if (cur_task->fd_array[i] == NULL)
        {
            cur_task->fd_array[i] = file;
            trapframe->x[0] = i;
            return;
        }

    trapframe->x[0] = -1;
}

void sys_close(struct ucontext *trapframe)
{
    int fd = trapframe->x[0];

    struct task_struct* cur_task = get_current_task();
    trapframe->x[0] = vfs_close(cur_task->fd_array[fd]);
    cur_task->fd_array[fd] = NULL;
}

void sys_write(struct ucontext *trapframe)
{
    int fd = (int)trapframe->x[0];
    const void *buf = (const void *)trapframe->x[1];
    unsigned long count = (unsigned long)trapframe->x[2];

    struct task_struct* cur_task = get_current_task();
    trapframe->x[0] = vfs_write(cur_task->fd_array[fd], buf, count);
}

void sys_read(struct ucontext *trapframe)
{
    int fd = (int)trapframe->x[0];
    void *buf = (void *)trapframe->x[1];
    unsigned long count = (unsigned long)trapframe->x[2];

    struct task_struct* cur_task = get_current_task();
    trapframe->x[0] = vfs_read(cur_task->fd_array[fd], buf, count);
}

void sys_mkdir(struct ucontext *trapframe)
{
    const char *pathname = (const char *)trapframe->x[0];

    /*uart_puts("mkdir: ");
    uart_puts(pathname);
    uart_puts("\n");*/
    
    trapframe->x[0] = vfs_mkdir(pathname);
}

void sys_mount(struct ucontext *trapframe)
{
    const char *src = (const char *)trapframe->x[0];
    const char *target = (const char *)trapframe->x[1];
    const char *filesystem = (const char *)trapframe->x[2];

    /*uart_puts("mkdir: ");
    uart_puts(target);
    uart_puts("\n");*/

    trapframe->x[0] = vfs_mount(src, target, filesystem);
}

void sys_chdir(struct ucontext *trapframe)
{
    const char *pathname = (const char *)trapframe->x[0];

    /*uart_puts("chdir: ");
    uart_puts(pathname);
    uart_puts("\n");*/

    trapframe->x[0] = vfs_chdir(pathname);
}

void sys_sigreturn(struct ucontext *trapframe)
{
    unsigned long long *sp_ptr = (unsigned long long *)trapframe->sp_el0;

    for (int i = 0; i < 34; i++)
        *((unsigned long long *)trapframe + i) = *(sp_ptr + (34 - i));
}