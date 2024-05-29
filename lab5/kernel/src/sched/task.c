#include <kernel/bsp_port/irq.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <lib/stdlib.h>

#include "kernel/timer.h"

// static unsigned long nr_count = 1;  // init_task has pid 0
// static task_struct_t init_task = INIT_TASK;
// static task_struct_t *run_queue = &(init_task);  // circular doubly linked
// list

static unsigned long nr_count = 0;       // init_task has pid 0
static task_struct_t *run_queue = NULL;  // circular doubly linked list

extern void cpu_switch_to(task_struct_t *prev, task_struct_t *next);
extern task_struct_t *get_current();

unsigned long get_new_pid() { return nr_count++; }

void preempt_disable() {
    task_struct_t *current = get_current();

    if (current == NULL) {
        return;
    }

    current->preempt_count++;

    // print_string("preempt_disable: ");
    // print_h((unsigned long)current);
    // print_string(" ");
    // print_d(current->pid);
    // print_string(" ");
    // print_d(current->preempt_count);
    // print_string("\n");
}

void preempt_enable() {
    task_struct_t *current = get_current();

    if (current == NULL) {
        return;
    }

    current->preempt_count--;
    // print_string("preempt_enable: ");
    // print_h((unsigned long)current);
    // print_string(" ");
    // print_d(current->pid);
    // print_string(" ");
    // print_d(current->preempt_count);
    // print_string("\n");
}

void enqueue_run_queue(task_struct_t *task) {
    // print_string("[enqueue_run_queue] begin \n");
    // print_task_list();
    if (run_queue == NULL) {
        run_queue = task;
        task->next = task;
        task->prev = task;
    } else {
        task_struct_t *prev = run_queue->prev;
        prev->next = task;
        task->prev = prev;
        task->next = run_queue;
        run_queue->prev = task;
    }
    // print_string("[enqueue_run_queue] done\n");
    // print_task_list();
}

void delete_run_queue(task_struct_t *task) {
    if (task->next == task) {
        run_queue = NULL;
    } else {
        task_struct_t *prev = task->prev;
        task_struct_t *next = task->next;
        prev->next = next;
        next->prev = prev;
        if (run_queue == task) {
            run_queue = next;
        }
    }
}

static void switch_to(task_struct_t *next) {
    if(get_current() == next) {
        // return;
        print_string("[switch_to] same task\n");
    }
#ifdef SCHED_DEBUG
    print_string("[switch_to] ");
    print_string("(");
    print_d(get_current()->pid);
    print_string(", ");
    print_d(get_current()->counter);
    print_string(", ");
    print_d(get_current()->priority);
    print_string(", ");
    print_d(get_current()->preempt_count);
    print_string(", ");
    print_h((unsigned long)get_current()->context.x19);
    print_string(", ");
    print_h((unsigned long)get_current()->context.x20);
    print_string(", ");
    print_h((unsigned long)get_current()->context.sp);
    // print_string(", ");
    // print_h((unsigned long)get_current()->context.pc);
    print_string(") -> (");
    // print_string(" -> ");
    // print_h((unsigned long)next);
    // print_string(", ");
    print_d(next->pid);
    print_string(", ");
    print_d(next->counter);
    print_string(", ");
    print_d(next->priority);
    print_string(", ");
    print_d(next->preempt_count);
    print_string(", ");
    print_h((unsigned long)next->context.x19);
    print_string(", ");
    print_h((unsigned long)next->context.x20);
    print_string(", ");
    print_h((unsigned long)next->context.sp);
    // print_string(", ");
    // print_h((unsigned long)next->context.pc);
    print_string(")");
    print_string("\n");

    print_string("[switch_to] print_task_list\n");
    print_task_list();
// }
#endif

    if (get_current() == next) {
        return;
    }

    cpu_switch_to(get_current(), next);
}

void print_task_list() {
    task_struct_t *task = get_current();
    print_string("task list: ");
    do {
        print_string("(");
        print_h((unsigned long)task);
        print_string(", ");
        print_d(task->pid);
        print_string(", ");
        print_d(task->counter);
        print_string(", ");
        print_d(task->priority);
        print_string(", ");
        print_h((unsigned long)task->context.x19);
        print_string(", ");
        print_h((unsigned long)task->context.x20);
        print_string(", ");
        print_h((unsigned long)task->context.pc);
        print_string(", ");
        print_h((unsigned long)task->context.sp);
        print_string(") -> ");
        task = task->next;
    } while (task != get_current());
    if (task == get_current()) print_string("Circular\n");
    print_string("\n");
}

void _schedule() {
    preempt_disable();
    task_struct_t *current_task;
    task_struct_t *next_task = NULL;
    task_struct_t *start_point = run_queue;

    int highest_counter = -1;
    while (1) {
        current_task = start_point;
        do {
            // find next highest counter task
            if (current_task->state == TASK_RUNNING &&
                current_task->counter > highest_counter &&
                current_task->context.pc != 0) {
                // ) {
                highest_counter = current_task->counter;
                next_task = current_task;

#ifdef SCHED_DEBUG
                print_string("[_schedule] got new task: ");
                print_h((unsigned long)next_task);
                print_string(", ");
                print_h(next_task->context.pc);
                print_string("\n");
#endif
            }
            // print_string("current_task: ");
            // print_d(current_task->pid);
            // print_string(", counter: ");
            // print_d(current_task->counter);
            // print_string("\n");
            current_task = current_task->next;  // 移至下一个节点
            // print_string("current_task->next: ");
            // print_d(current_task->pid);
            // print_string("\n");
        } while (current_task != start_point);  // 循环直到回到起点

        if (highest_counter > 0) {
            // print_string("[_schedule] highest_counter: ");
            // print_d(highest_counter);
            // print_string("\n");
            break;
        }

        current_task = start_point;
        do {
            current_task->counter =
                (current_task->counter >> 1) + current_task->priority;
            current_task = current_task->next;  // 重新遍历整个链表，更新优先级
        } while (current_task != start_point);
    }

#ifdef SCHED_DEBUG
    print_string("[_schedule] switch_to: ");
    print_h((unsigned long)next_task);
    print_string("\n");
#endif

    switch_to(next_task);
    preempt_enable();
}

void schedule() {
    get_current()->counter = 0;
    _schedule();

#ifdef SCHED_DEBUG
    print_string("[schedule]\n");
    print_task_list();
#endif
}

void timer_tick() {
    task_struct_t *current = get_current();
    // print_string("-------\ntimer tick: ");
    // print_d(current->pid);
    // print_string(", priority: ");
    // print_d(current->priority);
    // print_string(", counter: ");
    // print_d(current->counter);
    // print_string(", preempt_count: ");
    // print_d(current->preempt_count);
    // print_string("\n------\n");

    if (--current->counter > 0 || current->preempt_count > 0) {
        return;
    }
    current->counter = 0;

    enable_irq();
    _schedule();
    disable_irq();
}

void schedule_tail() {
    // print_string("[schedule_tail]\n");
    preempt_enable();
}

task_struct_t *get_task(int pid) {
    task_struct_t *task = run_queue;
    do {
        if (task->pid == pid) {
            return task;
        }
        task = task->next;
    } while (task != run_queue);
    return NULL;
}

static void _kill_zombies() {
    task_struct_t *task = run_queue;
    do {
        if (task->state == TASK_ZOMBIE) {
            task_struct_t *next = task->next;
            delete_run_queue(task);
            kfree(task);
            task = next;
        } else {
            task = task->next;
        }
    } while (task != run_queue);
}

void root_task() {
    print_string("root task\n");
    while (1) {
        _kill_zombies();
        schedule();
    }
}

void sched_init() {
    print_string("[sched init]\n");

    static task_struct_t init_task = INIT_TASK;

    enqueue_run_queue(&init_task);
    nr_count = 1;

    copy_process(PF_KTHREAD, (unsigned long)&root_task, 0, 0);
}

void exit_process() {
    preempt_disable();

    task_struct_t *current = get_current();
    current->state = TASK_ZOMBIE;
    if (current->stack) {
        kfree((void *)current->stack);
    }

    preempt_enable();
    schedule();
}

void kill_process(long pid) {
    task_struct_t *task = get_task(pid);
    if (task) {
        task->state = TASK_ZOMBIE;
        schedule();
    }
}
