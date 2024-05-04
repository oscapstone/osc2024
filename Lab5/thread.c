#include "memory.h"
extern switch_to;
int p_count = 0;

struct registers {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long lr;
	unsigned long sp;
};

struct thread{
    struct registers regs;
    int pid;
    int state; //run queue, running, wait queue...
    int parent;
    int priority;
    unsigned long stack_pointer;
};

#define MAX_TASK 64
struct thread * thread_pool[MAX_TASK];

int create_thread(void * function){
    int pid;
    struct thread * t = malloc(sizeof(struct thread));
    for(int i=0; i<MAX_TASK; i++){
        if(thread_pool[i] == 0){
            pid = i;

        }
    }
    
}
