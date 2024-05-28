
#include "lib/fork.h"
#include "lib/printf.h"
#include "lib/getpid.h"

static char test_data[100]; // bss section

int main(int argc, char** argv) {

    int pid = fork();

    int i = 10;  // stack

    if (pid == 0) {
        i = 20;
        pid = getpid();
        sprintf(test_data, "child process gogo\n");             // writing data that is not in stack trigger page fault handler to do copy on write
        printf(test_data);
        printf("child process: pid = %d, i = %d\n", pid, i);
    } else {
        i = 100;
        int mypid = getpid();
        sprintf(test_data, "parent process gogo\n");
        printf(test_data);
        printf("parent process: pid = %d, child pid = %d, i = %d\n", mypid, pid, i);
    }

    return 0;
}

