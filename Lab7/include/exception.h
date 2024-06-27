#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

typedef struct trapframe{
  unsigned long x[31];
  unsigned long spsr_el1;
  unsigned long elr_el1;
  unsigned long sp_el0;
}trapframe;

int exception_entry();
int lower_exception_entry();

#endif