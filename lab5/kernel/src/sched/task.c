#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <lib/string.h>

static int thread_count = 0;
static struct task_struct *run_queue;

extern void switch_to(struct task_struct *prev, struct task_struct *next);

static void enqueue(struct task_struct **queue, struct task_struct *task) {
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

static void remove(struct task_struct **queue, struct task_struct *task) {
    if (*queue == task) *queue = (task->next == task) ? 0 : task->next;
    task->next->prev = task->prev;
    task->prev->next = task->next;
}

void display_run_queue() {
    struct task_struct *task = run_queue;
    do {
        print_string("Task id: ");
        print_d(task->pid);
        print_string(", state: ");
        print_d(task->state);
        print_string(", next: ");
        print_d(task->next->pid);
        print_string("\n");
        task = task->next;
    } while (task != run_queue);
}

void schedule() {
    struct task_struct *current = get_current();
    struct task_struct *next = current->next;

    while(next->state != TASK_RUNNING) {
        next = next->next;
    }

    // display_run_queue();
    // print_string("Scheduling from: ");
    // print_d(current->pid);
    // print_string(" -> ");
    // print_d(next->pid);
    // print_string("\n");

    switch_to(current, current->next);
}

void kill_zombies() {
    // print_string("Killing zombies\n");
    // display_run_queue();
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

void idle() {
    while (1) {
        kill_zombies();
        schedule();
    }
}

struct task_struct *get_task(int pid) {
    struct task_struct *task = run_queue;
    do {
        if (task->pid == pid) return task;
        task = task->next;
    } while (task != run_queue);
    return 0;
}

void kthread_init() {
    kthread_create(idle);
    asm volatile("msr tpidr_el1, %0" ::"r"(run_queue));
    // display_run_queue();
}

struct task_struct *kthread_create(void (*func)()) {
    struct task_struct *task = kmalloc(sizeof(struct task_struct));
    task->pid = thread_count++;
    task->state = TASK_RUNNING;
    task->stack = kmalloc(STACK_SIZE);
    task->user_stack = kmalloc(STACK_SIZE);
    memset(task->sighand, 0, sizeof(task->sighand));
    task->sigpending = 0;
    task->sighandling = 0;
    task->context.lr = (unsigned long)func;
    task->context.sp = (unsigned long)task->user_stack + STACK_SIZE;
    task->context.fp = (unsigned long)task->user_stack + STACK_SIZE;
    enqueue(&run_queue, task);
    return task;
}

void kthread_exit() {
    get_current()->state = EXIT_ZOMBIE;
    schedule();
}

void kthread_stop(int pid) {
    struct task_struct *task = run_queue;
    do {
        if (task->pid == pid) task->state = EXIT_ZOMBIE;
        task = task->next;
    } while (task != run_queue);
    schedule();
}

void thread_test() {
    for (int i = 0; i < 5; ++i) {
        // uart_puts("Thread id: ");
        // uart_hex(get_current()->pid);
        // uart_puts(" ");
        // uart_hex(i);
        // uart_puts("\n");
        for (int i = 0; i < 1000000; i++);
        schedule();
    }
    kthread_exit();
}
