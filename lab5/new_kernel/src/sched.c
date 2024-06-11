#include "memory.h"
#include "timer.h"
#include "utility.h"
#include "stdint.h"
#include "syscall.h"
#include "exception.h"
#include "mini_uart.h"
#include "shell.h"
#include "sched.h"

list_head_t *run_queue;

thread_t *threads[MAX_PID + 1];
thread_t *curr_thread;

static int64_t pid_history = 0;
int8_t need_to_schedule = 0;

void init_thread_sched()
{
    lock();

    run_queue = kmalloc(sizeof(thread_t));
    INIT_LIST_HEAD(run_queue);

    // idle process?
    char *thread_name = kmalloc(5);

    // init process
    // thread_name = kmalloc(5);
    // strcpy(thread_name, "init");
    // thread_t *init_thread = thread_create(init, thread_name);
    // init_thread->datasize = 0x4000;
    // curr_thread = init_thread;
};
thread_t *thread_creat(char *name, void *start)
{
    lock();
    thread_t *t;
    int64_t new_pid = -1;

    for (int i = 1; i < MAX_PID; i++)
    {
        if (threads[pid_history + i] == NULL)
        {
            new_pid = pid_history + i;
            break;
        }
    }
}
void foo(){
    for(int i = 0; i < 10; ++i) {
        // printf("Thread id: %d %d\n", current_thread().id(), i);
        // delay(1000000);
        schedule();
    }
};

void schedule() {
    // current thread 換成 run queue->next
    lock();
    thread_t *prev_thread = curr_thread;
    while (list_is_head((list_head_t *)curr_thread, run_queue)){
        curr_thread = (thread_t *) (((list_head_t *)curr_thread)->next);
    }
    curr_thread->status = THREAD_IS_RUNNING;
    unlock();
    switch_to(get_current_thread_context(), &(curr_thread->context));

    //
};
void idle(){
    //  while True:
    //     kill_zombies() # reclaim threads marked as DEAD
    //     schedule() # switch to any other runnable thread
};