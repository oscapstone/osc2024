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

    printf("[default_sigkill] default SIGKILL handler in user space, task %d be killed.\n", pid);

    return;
}

struct sighandler default_sighandler = {
    .action = {no_sighandler, no_sighandler, no_sighandler, no_sighandler, no_sighandler,
                no_sighandler, no_sighandler, no_sighandler, no_sighandler, default_sigkill}
};