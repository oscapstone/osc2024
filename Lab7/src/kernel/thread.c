#include "kernel/thread.h"

thread_t *cur_thread;
thread_t *run_queue;
thread_t *wait_queue;

void thread_init(void){
    lock();

    cur_thread = 0;
    run_queue = 0;
    wait_queue = 0;

    // tpidr:Thread Pointer Identifier Register
    // Allocate a space that can hold a Thread Control Block(TCB)
    asm volatile(
        "msr tpidr_el1, %[var1];"
        :
        :[var1] "r" (pool_alloc(sizeof(thread_t)))
    );
    
    cur_thread = thread_create(idle_task, 0);
    unlock();
}

thread_t* thread_create(void *fn, void *arg){
    // record the pid that already allocated(no matter that thread is alive or not)
    static int allocated_pid = 0;

    thread_t *new_thread = (thread_t*)pool_alloc(2048);
    if(new_thread == 0)
        return 0;
    uart_b2x_64(sizeof(thread_context_t));
    memzero((my_uint64_t)&new_thread->context, sizeof(struct thread_context));
    
    new_thread->data = arg;
    new_thread->pid = allocated_pid++;
    new_thread->next = 0;
    new_thread->prev = 0;
    new_thread->status = 0; // consider this thread is waiting
    
    new_thread->sp = (void*)(pool_alloc(THREAD_STK_SIZE));
    // set stack pointer to the end of this process's stack
    // But won't this corrupt other memory regions? -> Well... it's stack, so it is growing downward, which means it won't corrupt other memory regions.
    new_thread->context.sp = (unsigned long)new_thread->sp + THREAD_STK_SIZE;
    // set the frame pointer to the value of the stack pointer just before the function was called
    new_thread->context.fp = new_thread->context.sp;
    // store function in link register, which will be executed after return from 'switch_to' 
    new_thread->context.lr = (unsigned long)fn;
    
    if(new_thread->sp == 0){
        pool_free(new_thread);
        return 0;
    }

    lock();

    if(run_queue == 0) // if no thread is running, make this thread as running thread
        run_queue = new_thread;
    else{               // else put this thread to the end of run queue
        thread_t *current = run_queue;
        while(current->next != 0)
            current = current->next;
        current->next = new_thread;
        new_thread->prev = current;
    }

    unlock();

    return new_thread;
}

void schedule(void){
    if(run_queue == 0){
        uart_puts("No thread can be schedule(only idle)\n");
        return;
    }

    lock();
    // if no thread is running(which is unlikely as there's an idle thread), make the first thread in run queue as running thread
    if(cur_thread == 0){
        uart_puts("No thread is running\n");
        cur_thread = run_queue;
    }
    else if(cur_thread->next != 0 && cur_thread->next->status != -1) // not zombie
        cur_thread = cur_thread->next;
    // make it circular
    else if(cur_thread->next == 0 && run_queue->status != -1)
        cur_thread = run_queue;
    else if(cur_thread->next == 0 && run_queue->status == -1){
        uart_puts("No thread can be schedule(only idle)\n");
        return;
    }
    
    cur_thread->status = 1; // running
    //uart_puts("Switch to thread: ");
    //uart_itoa(cur_thread->pid);

    switch_to(get_current(), &cur_thread->context);

    unlock();
}

void idle_task(void){
    //static int i = 0;
    while(1){
        kill_zombies();
        schedule();
        //i++;
    }
}
// reclaim threads marked as zombie. In this exercise, all threads are consider the child of idle thread
void kill_zombies(void){
    thread_t *current = run_queue;
    while(current != 0){
        if(current->status == -1){
            thread_t *tmp = current;
            uart_puts("Zombie killed: ");
            uart_itoa(tmp->pid);
            uart_putc('\n');
            current = current->next;
            pool_free(tmp->sp);
            pool_free(tmp);
        }
        else
            current = current->next;
    }
}

void foo(void){
    for(int i = 0; i < 10; ++i) {
        uart_puts("Thread id: ");
        uart_itoa(cur_thread->pid);
        uart_putc(' ');
        uart_itoa(i);
        uart_putc('\n');
        delay(1000000);
        schedule();
    }
}
