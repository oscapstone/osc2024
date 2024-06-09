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

void output_lockstate(){
    puts("lock_count:");
    put_int(lock_count);
    puts("\r\n");
    return;
}

void output_daif(){
    puts("DAIF:0x");
	unsigned int DAIF = 0;
	asm volatile("mrs %0, DAIF":"=r"(DAIF));
    DAIF = DAIF >> 6;
	put_hex(DAIF);

	puts("\r\n");
    return;
}