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
    bool periodic;
    struct list_head list;
} timeout_event_t;

LIST_HEAD(timeout_event_head);

void set_core_timer_timeout(unsigned int seconds)
{
    unsigned long freq = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    asm volatile("msr cntp_tval_el0, %0" ::"r"(seconds * freq));
}


static inline void insert_timeout_event(timeout_event_t* new_event)
{
    unsigned int invoke_time = new_event->reg_time + new_event->duration;

    // if the list is empty
    if (list_empty(&timeout_event_head)) {
        list_add(&new_event->list, &timeout_event_head);
        set_core_timer_timeout(new_event->duration);
        enable_core0_timer();
        return;
    }

    timeout_event_t* node;

    list_for_each_entry (node, &timeout_event_head, list) {
        if (node->reg_time + node->duration > invoke_time) {
            list_add_tail(&new_event->list, &node->list);
            break;
        }
    }

    // if insert at the end of the list
    if (&node->list == &timeout_event_head) {
        list_add_tail(&new_event->list, &timeout_event_head);
        return;
    }

    // if insert at the beginning of the list
    if (new_event->list.prev == &timeout_event_head) {
        set_core_timer_timeout(new_event->duration);
        enable_core0_timer();
    }
}

void add_timer(timer_callback cb,
               char* msg,
               unsigned int duration,
               bool periodic)
{
    timeout_event_t* new_timeout_event =
        (timeout_event_t*)mem_alloc(sizeof(timeout_event_t));

    if (!new_timeout_event)
        return;

    INIT_LIST_HEAD(&new_timeout_event->list);
    new_timeout_event->reg_time = get_current_time();
    new_timeout_event->duration = duration;
    new_timeout_event->callback = cb;
    new_timeout_event->periodic = periodic;

    new_timeout_event->msg = mem_alloc(sizeof(char) * 20);

    if (!new_timeout_event->msg)
        return;

    str_n_cpy(new_timeout_event->msg, msg, 20);

    insert_timeout_event(new_timeout_event);
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

    /* periodic timer */
    if (first->periodic) {
        first->reg_time = current_time;
        insert_timeout_event(first);
    }

    if (list_empty(&timeout_event_head)) {
        disable_core0_timer();
        return;
    }

    timeout_event_t* next =
        list_first_entry(&timeout_event_head, timeout_event_t, list);

    enable_core0_timer();
    set_core_timer_timeout(next->reg_time + next->duration -
                           get_current_time());
}


void print_msg(char* msg)
{
    uart_send_string(msg);
    uart_send_string("\n");
}

void set_timeout(char* msg, unsigned int duration, bool periodic)
{
    add_timer(print_msg, msg, duration, periodic);
}
