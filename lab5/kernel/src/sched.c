#include "sched.h"
#include "memory.h"

list_head_t *run_queue;
thread_t threads[PIDMAX + 1];
thread_t *curr_thread;

void init_thread_sched() {
    lock();
    run_queue = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(run_queue);

    for (int i=0; i<=PIDMAX; i++) {
        threads[i].pid = i;
        threads[i].status = THREAD_READY;
    }

}

thread_t *_init_create_thread(char *name, int64_t pid, int64_t ppid, void *start) {
    thread_t *thread = (thread_t *)kmalloc(sizeof(thread_t));

}
