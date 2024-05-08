#include "signal.h"
#include "uart.h"
#include "syscall.h"

void no_sighandler(void) {
    return;
}

void default_sigkill(void) {
    printf("SIGKILL handler in user space\n");
    return;
}

void sigreturn(void)
{
    printf("[sigreturn] Task %d return from signal handler\n", get_taskid());
    return;
}


struct sighandler default_sighandler = {
    .action = {no_sighandler, no_sighandler, no_sighandler, no_sighandler, no_sighandler,
                no_sighandler, no_sighandler, no_sighandler, no_sighandler, default_sigkill}
};