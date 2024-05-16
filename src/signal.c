#include "signal.h"
#include "uart.h"
#include "syscall.h"
#include "sched.h"

void no_sighandler(void) {
    printf("[no_sighandler] Do nothing in user space\n");
    return;
}

void default_sigkill(void) {
    int pid = get_taskid();

    task_pool[pid].state = TASK_STOPPED;
    task_pool[pid].exit_state = 0;
    num_running_task--;

    return;
}

struct sighandler default_sighandler = {
    .action = {no_sighandler, no_sighandler, no_sighandler, no_sighandler, no_sighandler,
                no_sighandler, no_sighandler, no_sighandler, no_sighandler, default_sigkill}
};