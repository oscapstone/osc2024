#include "include/thread.h"
#include "include/dynamic_allocator.h"
#include "include/page_allocator.h"

ThreadQueue thread_queue;

void init_thread(){
    // create idle thread
    for(int i=0;i<MAX_THREAD;i++){
        thread_queue.queue[i].available = KILLED;
    }
    thread_queue.current_routine = 0;
    thread_queue.queue_index = 0;
}


int creat_thread(void (*function_ptr)()){
    for(int i=1;i<MAX_THREAD;i++){
        if(thread_queue.queue[i].available == KILLED){
            thread_queue.queue[i].available = READY;
            // malloc new user stack using allocator in lab 4
            thread_queue.queue[i].user_stack = (uint16_t* )malloc(USER_STACK_SIZE);
            // thread_queue.queue[i].kernel_stack = (uint16_t* )malloc(KERNEL_STACK_SIZE);
            thread_queue.queue[i].tss.x19 = 0x00;
            thread_queue.queue[i].tss.x20 = 0x00;
            thread_queue.queue[i].tss.x21 = 0x00;
            thread_queue.queue[i].tss.x22 = 0x00;
            thread_queue.queue[i].tss.x23 = 0x00;
            thread_queue.queue[i].tss.x24 = 0x00;
            thread_queue.queue[i].tss.x25 = 0x00;
            thread_queue.queue[i].tss.x26 = 0x00;
            thread_queue.queue[i].tss.x27 = 0x00;
            thread_queue.queue[i].tss.x28 = 0x00;
            thread_queue.queue[i].tss.fp = (uint64_t)(thread_queue.queue[i].user_stack+USER_STACK_SIZE); // x29: base pointer for local variable in stack
            thread_queue.queue[i].tss.lr = (uint64_t)function_ptr; // x30: store return address, jump to the target function
            thread_queue.queue[i].tss.sp = thread_queue.queue[i].tss.fp; // stack pointer, varys from function calls
            uart_puts("[Creat Thread] new thread id ");
            uart_hex(i);
            uart_puts(" created\n");
            // return id
            return i;
        }
    }
    uart_puts("[Thread creat fail] Thread Pool full\n");
    return -1;
}



// /* Find empty task_struct. Return task id. */
// int find_empty_task(){
//     int i;
//     for (i = 1; i < NR_TASKS; i++)
//         if (task_pool[i].state == TASK_STOPPED)
//             break;
//     if (i != NR_TASKS)
//         num_running_task++;
//     return i;
// }

// exit the thread
void exit(){
    struct Thread* current_thread = get_current();
    int id = 0;
    // find the id of current thread
    for(int i=0;i<MAX_THREAD;i++){
        if(&thread_queue.queue[i] == current_thread){
            id = i;
            break;
        }
    }
    uart_puts("thread id 0x");
    uart_hex(id);
    uart_puts(" has exit\n");
    current_thread->available = EXIT;
    schedule();
}

void schedule(){
    int id = get_id_from_struct_thread(get_current());
    // find next READY thread start from schedule caller id
    for(int i=1; i< MAX_THREAD; i++){
        int next = (id+i)%MAX_THREAD;
        // skip idle thread(id = 0)
        if(next == 0){
            continue;
        }
        else if(thread_queue.queue[next].available == READY){
            thread_queue.current_routine = next;
            context_switch(id, next);
            return;
        }
    }
    context_switch(id, 0);
    return;
}


/* Do context switch. */
void context_switch(int prev_id, int next_id){
    // uart_puts("[Context switch] Switch from thread id 0x");
    // uart_hex(prev_id);
    // uart_puts(" to thread id 0x");
    // uart_hex(next_id);
    // uart_puts("\n");
    switch_to(&thread_queue.queue[prev_id], &thread_queue.queue[next_id]);
}

int get_id_from_struct_thread(struct Thread* cur){
    // find the id of current thread
    for(int i=0;i<MAX_THREAD;i++){
        if(&thread_queue.queue[i] == cur){
            return i;
        }
    }
    uart_puts("[Assertion error] Thread not exist\n");
    return -1;
}

void foo(){
    for(int i = 0; i < 10; ++i) {
        int id = get_id_from_struct_thread(get_current());

        uart_puts("Thread id: ");
        uart_hex(id);
        uart_puts(" ");
        uart_hex(i);
        uart_puts("\n");
        int r=1000000; while(r--) { asm volatile("nop"); }
        schedule();
    }
    exit();
}

void idle() {
    // no other threads are running now
    uart_puts("Enter idle thread\n");
    while(1){
        for(int i=1;i<MAX_THREAD;i++){
            if(thread_queue.queue[i].available == EXIT){
                uart_puts("[FREE Thread] thread id 0x");
                uart_hex(i);
                uart_puts(" is recycled\n");
                free(thread_queue.queue[i].user_stack);
                thread_queue.queue[i].available = KILLED;
                thread_queue.queue_index--;
            }
        }
        // switch to other thread
        schedule();
    }
}