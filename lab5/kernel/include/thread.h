#ifndef THREAD_H
#define THREAD_H

typedef struct thread {
	unsigned long long reg[10];
	void* fp;
	void* lr;
	void* sp;
	void* stack_start;
	int state;
} thread;

typedef struct thread_node {
	thread* thread;
	struct thread_node* next;
	int id;
} thread_node;

void thread_init();
void thread_test();
void kill_zombies();
void idle();
void schedule();

#endif
