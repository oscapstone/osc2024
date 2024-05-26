#ifndef THREAD_H
#define THREAD_H

typedef struct thread {
	unsigned long long reg[10];
	void* fp;
	void* lr;
	void* sp;
	void* stack_start;
	void* kstack_start;
	int state, id;
	struct thread* next;
	struct thread* prev;
} thread;

void thread_init();
void thread_test();
void kill_zombies();
void idle();
void schedule();
int thread_create(void*);
void kill_thread(int);

#endif
