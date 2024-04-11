#ifndef _UTLI_H
#define _UTLI_H

unsigned int get(volatile unsigned int *addr);
void set(volatile unsigned int *addr, unsigned int val);
void wait_cycles(int r);
void wait_usec(unsigned int n);

#endif