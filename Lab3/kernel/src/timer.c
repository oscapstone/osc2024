#include "timer.h"
#include "def.h"
#include "list.h"
#include "memory.h"
#include "mini_uart.h"
#include "string.h"

typedef struct timeout_event {
    unsigned int reg_time;
    unsigned int duration;
    timer_callback callback;
    char* msg;
    struct list_head list;
} timeout_event_t;

LIST_HEAD(timeout_event_head);

static unsigned int seconds = 0;

unsigned int get_seconds(void)
{
    return seconds;
}

void set_seconds(unsigned int s)
{
    seconds = s;
}

void set_core_timer_timeout(void)
{
    unsigned long freq = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    asm volatile("msr cntp_tval_el0, %0" ::"r"(seconds * freq));
}


void add_timer(timer_callback cb, char* msg, unsigned int duration)
{
    timeout_event_t* new_timeout_event =
        (timeout_event_t*)mem_alloc(sizeof(timeout_event_t));

    INIT_LIST_HEAD(&new_timeout_event->list);
    new_timeout_event->reg_time = get_current_time();
    new_timeout_event->duration = duration;
    new_timeout_event->callback = cb;

    new_timeout_event->msg = mem_alloc(sizeof(char) * 20);
    str_n_cpy(new_timeout_event->msg, msg, 20);


    unsigned int new_timeout =
        new_timeout_event->reg_time + new_timeout_event->duration;

    if (list_empty(&timeout_event_head)) {
        list_add(&new_timeout_event->list, &timeout_event_head);
        set_seconds(new_timeout_event->duration);
        set_core_timer_timeout();
        enable_core0_timer();
        return;
    }

    timeout_event_t* node;

    list_for_each_entry (node, &timeout_event_head, list) {
        if (node->reg_time + node->duration > new_timeout) {
            list_add_tail(&new_timeout_event->list, &node->list);
            break;
        }
    }

    if (&node->list == &timeout_event_head)
        list_add_tail(&new_timeout_event->list, &timeout_event_head);

    if (new_timeout_event->list.prev == &timeout_event_head) {
        set_seconds(new_timeout_event->duration);
        set_core_timer_timeout();
        enable_core0_timer();
    }
}

void core_timer_handle_irq(void)
{
    unsigned int current_time = get_current_time();
    uart_send_string("\nmessage: ");

    timeout_event_t* first =
        list_first_entry(&timeout_event_head, timeout_event_t, list);

    first->callback(first->msg);

    uart_send_string("current time: ");
    uart_send_dec(current_time);
    uart_send_string("\n");

    uart_send_string("command execute time: ");
    uart_send_dec(first->reg_time);
    uart_send_string("\n");

    uart_send_string("command duration time: ");
    uart_send_dec(first->duration);
    uart_send_string("\n");

    list_del_init(&first->list);

    if (list_empty(&timeout_event_head)) {
        disable_core0_timer();
        return;
    }

    timeout_event_t* next =
        list_first_entry(&timeout_event_head, timeout_event_t, list);

    set_seconds(next->reg_time + next->duration - get_current_time());
    enable_core0_timer();
    set_core_timer_timeout();
}


void print_msg(char* msg)
{
    uart_send_string(msg);
    uart_send_string("\n");
}

void set_timeout(char* msg, unsigned int duration)
{
    add_timer(print_msg, msg, duration);
}

