#include "sched.h"
#include "mm.h"
#include "string.h"
#include "uart.h"

static int thread_count = 0;
static struct task_struct *run_queue;

extern void switch_to(struct task_struct *prev, struct task_struct *next);
extern void switch_mm(unsigned long pgd);

static void enqueue(struct task_struct **queue, struct task_struct *task)
{
    if (*queue == 0) {
        *queue = task;
        task->next = task;
        task->prev = task;
    } else {
        task->next = *queue;
        task->prev = (*queue)->prev;
        (*queue)->prev->next = task;
        (*queue)->prev = task;
    }
}

static void remove(struct task_struct **queue, struct task_struct *task)
{
    if (*queue == task)
        *queue = (task->next == task) ? 0 : task->next;
    task->next->prev = task->prev;
    task->prev->next = task->next;
}

void display_run_queue()
{
    struct task_struct *task = run_queue;
    do {
        uart_puts("Task id: ");
        uart_hex(task->pid);
        uart_puts(" ");
        uart_hex((intptr_t)task);
        uart_puts("\n");
        task = task->next;
    } while (task != run_queue);
}

void schedule()
{
    switch_mm((unsigned long)VIRT_TO_PHYS(get_current()->next->pgd));
    switch_to(get_current(), get_current()->next);
}

void kill_zombies()
{
    struct task_struct *next, *task = run_queue;
    do {
        next = task->next;
        if (task->state == EXIT_ZOMBIE) {
            remove(&run_queue, task);
            kfree(task->stack);
            kfree(task->user_stack);
        }
        task = next;
    } while (task != run_queue);
}

void idle()
{
    while (1) {
        kill_zombies();
        schedule();
    }
}

struct task_struct *get_task(int pid)
{
    struct task_struct *task = run_queue;
    do {
        if (task->pid == pid)
            return task;
        task = task->next;
    } while (task != run_queue);
    return 0;
}

void kthread_init()
{
    kthread_create(idle);
    asm volatile("msr tpidr_el1, %0" ::"r"(run_queue));
}

struct task_struct *kthread_create(void (*func)())
{
    struct task_struct *task = kmalloc(sizeof(struct task_struct));
    task->pid = thread_count++;
    task->state = TASK_RUNNING;
    task->start = func;
    task->code_size = 0;
    task->stack = kmalloc(STACK_SIZE);
    task->user_stack = kmalloc(STACK_SIZE);
    memset(task->sighand, 0, sizeof(task->sighand));
    task->sigpending = 0;
    task->siglock = 0;
    task->pgd = kmalloc(PAGE_SIZE);
    memset(task->pgd, 0, PAGE_SIZE);
    memset(task->cwd, 0, PATH_MAX);
    strncpy(task->cwd, "/", 1);
    memset(task->fdt, 0, sizeof(task->fdt));
    task->context.lr = (unsigned long)func;
    task->context.sp = (unsigned long)task->stack + STACK_SIZE;
    task->context.fp = (unsigned long)task->stack + STACK_SIZE;
    enqueue(&run_queue, task);
    return task;
}

void kthread_exit()
{
    get_current()->state = EXIT_ZOMBIE;
    schedule();
}

void kthread_stop(int pid)
{
    struct task_struct *task = run_queue;
    do {
        if (task->pid == pid)
            task->state = EXIT_ZOMBIE;
        task = task->next;
    } while (task != run_queue);
    schedule();
}

void thread_test()
{
    for (int i = 0; i < 5; ++i) {
        uart_puts("Thread id: ");
        uart_hex(get_current()->pid);
        uart_puts(" ");
        uart_hex(i);
        uart_puts("\n");
        for (int i = 0; i < 1000000; i++)
            ;
        schedule();
    }
    kthread_exit();
}
