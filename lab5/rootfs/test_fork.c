
#include "lib/fork.h"
#include "lib/uartwrite.h"

int main(int argc, char** argv) {

    int pid = fork();

    if (pid == 0) {
        uartwrite("is child proc\n", 14);
    } else {
        uartwrite("is parent proc\n", 15);
    }

    return 0;
}
