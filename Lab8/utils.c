#include <stddef.h>

void delay(int time){
    while(time--) { 
        asm volatile("nop"); 
    }
}

unsigned long long strlen(const char *str)
{
    size_t count = 0;
    while((unsigned char)*str++)count++;
    return count;
}

void memset(char * t, int size){
    for(int i=0;i<size;i++){
        t[i] = 0;
    }
}