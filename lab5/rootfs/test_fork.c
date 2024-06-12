
#include "lib/getpid.h"
#include "lib/fork.h"
#include "lib/uartwrite.h"
#include "lib/printf.h"

void delay(unsigned int val) {
    for(unsigned int i = 0;i < val; i++) {
        for (unsigned int j = 0; j < 100; j++) {
            asm volatile("nop");
        }
    }
}

int main(int argc, char** argv) {
    printf("\nFork Test, pid %d\n", getpid());
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        ++cnt;

        if ((ret = fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
                delay(10000);
                ++cnt;
            }
        }
        exit(0);
    }
    else {
        printf("parent here, pid %d, child %d\n", getpid(), ret);
    }

    return 0;
}
