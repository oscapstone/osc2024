#include "timer.h"

#include "interrupt.h"
#include "mem.h"
#include "string.h"
#include "uart1.h"
#include "utli.h"

static timer_event* te_head = (timer_event*)0;

void print_message(char* msg) {
  uart_send_string("\r\ntimeout message: ");
  uart_send_string(msg);
  uart_send_string(", ");
  print_timestamp();
}

void add_timer(timer_callback cb, char* msg, uint32_t sec) {
  timer_event* new_event = (timer_event*)malloc(sizeof(timer_event));

  if (!new_event) {
    uart_puts("add_timer: fail due to memory allocation fail");
    return;
  }

  new_event->next = (timer_event*)0;
  new_event->func = cb;
  new_event->expire_time = get_timestamp() + sec;
  new_event->message = (char*)malloc(strlen(msg) + 1);
  memcpy(new_event->message, msg, strlen(msg) + 1);

  core0_timer_interrupt_disable();
  if (!te_head || new_event->expire_time < te_head->expire_time) {
    new_event->next = te_head;
    te_head = new_event;
    set_core_timer_int_sec(sec);
  } else {
    timer_event* cur = te_head;
    while (1) {
      if (!cur->next || new_event->expire_time < cur->next->expire_time) {
        new_event->next = cur->next;
        cur->next = new_event;
        break;
      }
      cur = cur->next;
    }
  }
  core0_timer_interrupt_enable();
}

void timer_event_pop() {
  if (!te_head) {
    return;
  }

  timer_event* te = te_head;
  te->func(te->message);
  free(te->message);
  free(te);
  te_head = te_head->next;

  if (te_head) {
    set_core_timer_int_sec(te_head->expire_time - get_timestamp());
  }
}