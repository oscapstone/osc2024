#include "sched.h"
#include "mm.h"
#include "uart.h"

#define STACK_SIZE 4096

static int thread_count = 0;
static struct task_struct *run_queue;

void enqueue(struct task_struct **queue, struct task_struct *task)
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

void remove(struct task_struct **queue, struct task_struct *task)
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
    switch_to(get_current(), get_current()->next);
}

void kill_zombies()
{
    struct task_struct *task = run_queue;
    do {
        if (task->state == EXIT_ZOMBIE) {
            struct task_struct *next = task->next;
            remove(&run_queue, task);
            kfree((void *)task->context.sp - STACK_SIZE);
            kfree(task->stack - STACK_SIZE);
            task = next;
            continue;
        }
        task = task->next;
    } while (task != run_queue);
}

void idle()
{
    while (1) {
        kill_zombies();
        schedule();
    }
}

void kthread_init()
{
    kthread_create(idle);
    asm volatile("msr tpidr_el1, %0" ::"r"(run_queue));
}

struct task_struct *kthread_create(void (*func)())
{
    struct task_struct *task = kmalloc(sizeof(struct task_struct));
    task->context.lr = (unsigned long)func;
    task->context.sp = (unsigned long)kmalloc(STACK_SIZE) + STACK_SIZE;
    task->context.fp = task->context.sp;
    task->pid = thread_count++;
    task->state = TASK_RUNNING;
    task->stack = kmalloc(STACK_SIZE) + STACK_SIZE;
    enqueue(&run_queue, task);
    return task;
}

void kthread_exit()
{
    get_current()->state = EXIT_ZOMBIE;
    schedule();
}

void foo()
{
    for (int i = 0; i < 5; ++i) {
        uart_puts("Thread id: ");
        uart_hex(get_current()->pid);
        uart_puts(" ");
        uart_hex(i);
        uart_puts("\n");
        for (int i = 0; i < 500000000; i++)
            ;
        schedule();
    }
    kthread_exit();
}