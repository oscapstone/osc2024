#include <kernel/bsp_port/irq.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/task_queue.h>

typedef struct task_t {
    void (*func)();
    int priority;
    int dirty;  // Set to 1 if the task is under processing
    struct task_t *prev;
    struct task_t *next;
} task_t;

static task_t *task_head = 0;

void init_task_queue() {
    task_t *tmp = 0;
    task_head = tmp;
}

void print_task_queue() {
    task_t *tmp = task_head;
    int count = 0;
    print_string("\n----------------\n");
    while (tmp) {
        print_string("Task: ");
        print_d(count++);
        print_string(", Priority: ");
        print_d(tmp->priority);
        print_string(", Dirty: ");
        print_d(tmp->dirty);
        print_string("\n");
        tmp = tmp->next;
    }
    print_string("----------------\n");
}

void add_task(void (*func)(void), int priority) {
    task_t *new_task = (task_t *)simple_malloc(sizeof(task_t));
    // #ifdef DEBUG
    //     print_string("\n[add_task] task_t allocated at ");
    //     print_h((unsigned long)new_task);
    //     print_string("\n");
    // #endif /* ifdef DEBUG */
    new_task->func = func;
    new_task->priority = priority;
    new_task->dirty = 0;
    new_task->next = 0;
    new_task->prev = 0;

    if (!task_head) {
        task_head = new_task;
    } else {
        task_t *curr = task_head;
        while (curr->next && curr->priority <= priority) {
            curr = curr->next;
        }
        if (!curr->next && curr->priority <= priority) {
            curr->next = new_task;
            new_task->prev = curr;
        } else {
            new_task->next = curr;
            new_task->prev = curr->prev;
            if (!new_task->prev) task_head = new_task;
            curr->prev = new_task;
        }
    }

#ifdef TASK_DEBUG
    print_string("\[] ");
    print_string("Task added");
    print_task_queue();
#endif
}

void run_task() {
    while (task_head && !task_head->dirty) {
        el1_disable_interrupt();
        task_t *tmp = task_head;
        tmp->dirty = 1;
        el1_enable_interrupt();
        tmp->func();
        el1_disable_interrupt();
        if (tmp->prev) tmp->prev->next = tmp->next;
        if (tmp->next) tmp->next->prev = tmp->prev;
        if (task_head == tmp) task_head = tmp->next;
        el1_enable_interrupt();
    }
}
