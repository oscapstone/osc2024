#ifndef _UTLI_H
#define _UTLI_H

char *itox(int value, char *s);
char *itoa(int value, char *s);
char *ftoa(float value, char *s);
unsigned int align(unsigned int size, unsigned int s);
void align_inplace(unsigned int *size, unsigned int s);
unsigned long int get_timestamp();
unsigned int get(volatile unsigned int *addr);
void set(volatile unsigned int *addr, unsigned int val);
void reset();
void cancel_reset();
void power_off();
void wait_cycles(int r);
void wait_usec(unsigned int n);
void print_cur_el();
void print_el1_sys_reg();
#endif