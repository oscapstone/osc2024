#ifndef TIMER_H
#define TIMER_H

typedef void (*timer_callback)(char*);

typedef struct timer {
	struct timer* next;
	char data[100];
	timer_callback callback;
	unsigned long long exp;
} timer;

timer* create_timer(unsigned long long exp, char* str);
void print_data(char* str);
void set_timeout(unsigned long long s, char* str);

#endif
