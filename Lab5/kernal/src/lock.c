#include "lock.h"
#include "stdio.h"
static unsigned long long lock_count = 0;

void lock(void){
    asm volatile(
        "msr daifset, 0xf;"
    );
    lock_count++;
}

void unlock(void){
    lock_count--;
    if(lock_count == 0){
        asm volatile(
            "msr daifclr, 0xf;"
        );
    }
    else if(lock_count < 0){
        puts("Unlock without lock\n");
        while(1);
    }
}