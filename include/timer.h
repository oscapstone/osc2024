#ifndef _TIMER_H
#define _TIMER_H

typedef void (*timer_callback)(char* arg);

typedef struct timer_event {
  struct timer_event *prev, *next;
  unsigned int expire_time;
  char* arg;
  void (*func)(char*);
} timer_event;

void print_message(char* msg);
void add_timer(timer_callback, char* msg, unsigned int sec);
void timer_event_pop();
#endif
