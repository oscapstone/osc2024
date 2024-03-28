#include <stdlib.h>
#include <stdio.h>
int main(void){
    unsigned int* a = (volatile unsigned int*)0x20000;
    printf("%p", a);
}