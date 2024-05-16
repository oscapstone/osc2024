#include "timer.h"
#include "mini_uart.h"
#include "heap.h"
#include "memory.h"
#include "interrupt.h"
#include "string.h"

timer_t *timer_head = NULL;

uint64_t get_time_s(){
  return (uint64_t)get_time()/get_freq();
}

uint64_t get_time(){
  uint64_t now;
  asm volatile(
    "mrs %0, cntpct_el0;" : "=r" (now)
  );
  return now;
}

uint64_t get_freq(){
  uint64_t freq;
  asm volatile(
    "mrs %0, cntfrq_el0;" : "=r" (freq)
  );
  return freq;
}

void set_timeout(char* msg, uint64_t duration){
  uint64_t time = duration*get_freq();
  add_timer(time, print_message, (void*)strdup(msg));
}

int add_timer(uint64_t time, void (*callback)(void*), void* data){

  timer_t *new_timer = (timer_t*)kmalloc(sizeof(timer_t));

  if(new_timer == NULL){
    uart_sends("fail malloc\n");
    return -1;
  }

  new_timer->time = get_time()+time;
  new_timer->callback = callback;
  new_timer->data = data;
  new_timer->next = NULL;

  // disable_irq();
  timer_t* current = timer_head;
  timer_t* prev = NULL;

  while(current != NULL && current->time < new_timer->time){
    prev = current;
    current = current->next;
  }

  if(!prev){
    new_timer->next = timer_head;
    timer_head = new_timer;
  }
  else{
    new_timer->next = current;
    prev->next = new_timer;
  }
  // enable_irq();
  core_timer_enable();
  return 0;
}

void print_message(void *msg){
  char *message = (char*)msg;
  uart_sends("Timer message: ");
  uart_sends(message);
  uart_sends("\n");
}
