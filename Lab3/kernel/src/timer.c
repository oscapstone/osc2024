#include "timer.h"
#include "def.h"
#include "list.h"
#include "memory.h"
#include "mini_uart.h"
#include "string.h"

#define MSG_SIZE 32

static LIST_HEAD(timeout_event_head);

typedef struct timeout_event {
    unsigned long reg_time;   // register time, represent by ticks
    unsigned long duration;   // duration time, represent by ticks
    timer_callback callback;  // callback fucntion
    char* msg;                // callback fucntion argument
    bool is_periodic;         // is_preiodic
    struct list_head list;    // list node
} timeout_event_t;

static timeout_event_t* create_timeout_event(timer_callback cb,
                                             char* msg,
                                             unsigned long duration,
                                             bool is_periodic)
{
    timeout_event_t* new_timeout_event =
        (timeout_event_t*)mem_alloc(sizeof(timeout_event_t));

    if (!new_timeout_event)
        return NULL;

    INIT_LIST_HEAD(&(new_timeout_event->list));

    new_timeout_event->reg_time = get_current_ticks();
    new_timeout_event->duration = duration * get_freq();
    new_timeout_event->callback = cb;
    new_timeout_event->is_periodic = is_periodic;

    new_timeout_event->msg = mem_alloc(sizeof(char) * MSG_SIZE);

    if (!new_timeout_event->msg)
        return NULL;

    str_n_cpy(new_timeout_event->msg, msg, MSG_SIZE);

    return new_timeout_event;
}

static void insert_timeout_event(timeout_event_t* new_event)
{
    unsigned long invoke_time = new_event->reg_time + new_event->duration;

    timeout_event_t* node = NULL;
    list_for_each_entry (node, &timeout_event_head, list) {
        if (node->reg_time + node->duration > invoke_time)
            break;
    }

    list_add_tail(&new_event->list, &node->list);

    // if the new event is the first event in the list, set the core timer
    if (list_first_entry(&timeout_event_head, timeout_event_t, list) ==
        new_event) {
        set_core_timer_timeout_ticks(new_event->duration);
        enable_core0_timer();
    }
}

void add_timer(timer_callback cb,
               char* msg,
               unsigned long duration,
               bool is_periodic)
{
    timeout_event_t* new_timeout_event =
        create_timeout_event(cb, msg, duration, is_periodic);
    insert_timeout_event(new_timeout_event);
}

void core_timer_handle_irq(void)
{

    unsigned long current_ticks = get_current_ticks();
    unsigned long freq = get_freq();

    timeout_event_t* first_event =
        list_first_entry(&timeout_event_head, timeout_event_t, list);


    first_event->callback(first_event->msg);

    uart_send_string("current time: ");
    uart_send_dec(current_ticks / freq);
    uart_send_string("\n");

    uart_send_string("command execute time: ");
    uart_send_dec(first_event->reg_time / freq);
    uart_send_string("\n");

    uart_send_string("command duration time: ");
    uart_send_dec(first_event->duration / freq);
    uart_send_string("\n");

    list_del_init(&(first_event->list));

    /* periodic timer, insert current event back to the list */
    if (first_event->is_periodic) {
        first_event->reg_time = current_ticks;
        insert_timeout_event(first_event);
    }
    else
    {
        mem_free(first_event->msg);
        mem_free(first_event);
    }

    if (!list_empty(&timeout_event_head)) {
        timeout_event_t* next =
            list_first_entry(&timeout_event_head, timeout_event_t, list);

        set_core_timer_timeout_ticks(next->reg_time + next->duration -
                                     get_current_ticks());
        enable_core0_timer();
    }
}

void print_msg(char* msg)
{
    uart_send_string(msg);
    uart_send_string("\n");
}
