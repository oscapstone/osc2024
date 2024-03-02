#ifndef _UTLI_H
#define _UTLI_H

float get_timestamp();
unsigned int get(volatile unsigned int *addr);
void set(volatile unsigned int *addr, unsigned int val);
void reset();
void cancel_reset();
void wait_cycle(int r);
#endif