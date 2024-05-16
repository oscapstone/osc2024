#include "thread.h"
#include "dynamic_alloc.h"
#include "../peripherals/mini_uart.h"
#include "../peripherals/utils.h"

thread_t    thread_pool[MAX_THREADS];
Queue       run_queue;
Queue       wait_queue;
Queue       dead_queue;
thread_t*   cur_thread;

void init_thread_pool(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_pool[i].free = TRUE;
        thread_pool[i].pid = i;
        thread_pool[i].state = CREATED;
    }
}

void init_scheduler(void) {
    // Initialize run queue and wait queue.
    init_queue(&run_queue);
    init_queue(&wait_queue);
    init_queue(&dead_queue);
}

void schedule(void) {
    if (cur_thread != NULL && cur_thread->state != DEAD) {
        cur_thread->state = RUNNABLE;
        enqueue(&run_queue, cur_thread);
    }

    thread_t* next_thread = (thread_t*)dequeue(&run_queue);
    
    if (next_thread == NULL) {
        uart_send_string("No runnable thread!\r\n");
        return;
    }

    next_thread->state = RUNNING;

    // If no thread is running, branch to the thread and start running.
    if (cur_thread == NULL) {
        cur_thread = next_thread;
        asm volatile("msr tpidr_el1, %0" :: "r"(&cur_thread->context));
        asm volatile(
            "mov sp, %0\n"
            "blr %1\n"
            :
            : "r"(cur_thread->context.sp), "r"(cur_thread->context.lr)
        );
    } else {
        thread_t* prev_thread = cur_thread;
        cur_thread = next_thread;
        switch_to(&prev_thread->context, &cur_thread->context);
    }
}


thread_t* thread_create(void*(*start_routine)(void *)) {
    thread_t* new_thread = NULL;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].free) {
            new_thread = &thread_pool[i];
            break;
        }
    }

    // Initialize the components within the new thread.
    new_thread->free = FALSE;
    new_thread->user_stack_ptr = (char *)dynamic_alloc(USTACK);
    new_thread->kernel_stack_ptr = (char *)dynamic_alloc(KSTACK);
    // Link to the function the thread is supposed to execute.
    new_thread->context.lr = (uint64_t)start_routine;
    // Set sp to the top of user stack.
    new_thread->context.sp = (uint64_t)(new_thread->user_stack_ptr + USTACK);
    // This is where sp_el1 will point to during system call.
    new_thread->context.kernel_sp = (uint64_t)(new_thread->kernel_stack_ptr + KSTACK);
    new_thread->context.fp = new_thread->context.sp;
    new_thread->state = RUNNABLE;

    if (new_thread != NULL) {
        enqueue(&run_queue, (thread_t *)new_thread);
    }

    uart_send_string("thread in run queue: ");
    uart_send_int(queue_element_count(&run_queue));
    uart_send_string("\r\n");

    return new_thread;
}

void kill_zombies(void) {
    thread_t* t;

    while ((t = dequeue(&dead_queue)) != NULL) {
        t->state = CREATED;
        t->free = TRUE;

        // Free allocated stacks.
        dynamic_free((uint64_t)t->user_stack_ptr);
        dynamic_free((uint64_t)t->kernel_stack_ptr);
    }
}

void* foo(void* arg) {
    for(int i = 0; i < 10; ++i) {
        uart_send_string("Thread id: ");
        uart_send_int(cur_thread->pid);
        uart_send_string(" ");
        uart_send_int(i);
        uart_send_string("\r\n");
        delay(1000000);
        schedule();
    }
}

void idle_thread(void) {
    while (1) {
        kill_zombies();
        schedule();
    }
}

void exec_thread() {

}

