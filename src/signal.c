#include "signal.h"
#include "uart.h"
#include "syscall.h"
#include "sched.h"

void no_sighandler(void) {
    printf("[no_sighandler] Do nothing in user space\n");
    return;
}

void default_sigkill(void) {
    printf("[default_sigkill] default SIGKILL handler in user space\n");
    return;
}

struct sighandler default_sighandler = {
    .action = {no_sighandler, no_sighandler, no_sighandler, no_sighandler, no_sighandler,
                no_sighandler, no_sighandler, no_sighandler, no_sighandler, default_sigkill}
};