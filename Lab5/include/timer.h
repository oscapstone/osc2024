#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

typedef struct timer{
  uint64_t time;
  void (*callback)(void*);
  char msg[256];
  // void *msg;
  struct timer* next;
}timer_t;

extern timer_t *timer_head;

uint64_t get_time();
uint64_t get_freq();

void set_timeout(char* msg, uint64_t duration);
int add_timer(uint64_t time, void (*callback)(void*), void* msg);
void print_message(void *msg);

#endif