#include "timer.h"

#include "alloc.h"
#include "uart1.h"
#include "utli.h"

extern void core_timer_handler(unsigned int s);
extern void core0_timer_interrupt_enable();
extern void core0_timer_interrupt_disable();

timer_event* te_head = (timer_event*)0;

void print_message(char* msg) {
  uart_send_string("\r\nTimeout message: ");
  uart_send_string(msg);
  uart_send_string(", ");
  print_timestamp();
}

void add_timer(timer_callback cb, char* arg, unsigned int sec) {
  timer_event* new_event = (timer_event*)simple_malloc(sizeof(timer_event));

  if (!new_event) {
    uart_puts("add_timer: fail due to memory allocation fail");
    return;
  }

  new_event->next = (timer_event*)0;
  new_event->func = cb;
  new_event->expire_time = get_timestamp() + sec;
  new_event->arg = arg;

  core0_timer_interrupt_disable();
  if (!te_head || new_event->expire_time < te_head->expire_time) {
    new_event->next = te_head;
    te_head = new_event;
    core_timer_handler(sec);
  } else {
    timer_event* cur = te_head;
    while (1) {
      if (!cur->next || new_event->expire_time < cur->next->expire_time) {
        new_event->next = cur->next;
        cur->next = new_event;
        break;
      }
    }
  }
  core0_timer_interrupt_enable();
}

void timer_event_pop() {
  if (!te_head) {
    uart_puts("timer_event_pop: event list is empty.");
    core0_timer_interrupt_disable();
    return;
  }
  timer_event* te = te_head;
  te->func(te->arg);
  te_head = te_head->next;
  if (!te_head) {
    core0_timer_interrupt_disable();
  } else {
    core_timer_handler(te_head->expire_time - get_timestamp());
  }
}