#ifndef _UTLI_H
#define _UTLI_H

float get_timestamp();
unsigned int get(volatile unsigned int *addr);
void set(volatile unsigned int *addr, unsigned int val);
void reset();
void cancel_reset();
void power_off();
void wait_cycles(int r);
void wait_usec(unsigned int n);
#endif