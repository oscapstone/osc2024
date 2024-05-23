#include"memalloc.h"
#include"stdio.h"
char *allocated = &_heap_top;
void* simple_malloc(unsigned int size){

    allocated+=size;
    if(allocated > (char*)get_sp()){
        puts("error:heap overflow\r\n");
        allocated-=size;
        return 0;
    }
    return allocated-size;
}

void* get_sp() {
    void *sp;
    __asm__("mov %0, sp" : "=r"(sp));
    return sp;
}

char* mem_alin(char* ptr,int alin){
    if((unsigned long long)ptr%alin)ptr+=(alin-(unsigned long long)ptr%alin);
    return ptr;
}