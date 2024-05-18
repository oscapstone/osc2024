#ifndef EXCEPTION_H
#define EXCEPTION_H

typedef struct trapframe_t{
	unsigned long long x[32];
	// x[31] is empty
	unsigned long long spsr_el1;
	unsigned long long elr_el1;
} trapframe_t;
// basically all the shits stored by save_all

void exception_entry();
void core_timer_entry();

#endif
