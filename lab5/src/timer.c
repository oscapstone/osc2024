#include "../include/timer.h"
#include "../include/timer_utils.h"
#include "../include/list.h"
#include "../include/mem_utils.h"
#include "../include/string_utils.h"
#include "../include/mini_uart.h"
#include "../include/peripherals/mini_uart.h"
#include <stdint.h>

void enable_timer_interrupt()
{
    /*
        core_timer_enable:
        mov x0, 1
        msr cntp_ctl_el0, x0        // enable
        mrs x0, cntfrq_el0
        msr cntp_tval_el0, x0       // set expired time
        mov x0, 2
        ldr x1, =CORE0_TIMER_IRQ_CTRL
        str w0, [x1]                // unmask timer interrupt
    */
    write_sysreg(cntp_ctl_el0, 1);              // enable cpu timer
    // uint64_t frq = read_sysreg(cntfrq_el0);     // read cntfrq_el0 register
    // write_sysreg(cntp_tval_el0, frq * 2);       // set expired time
    *CORE0_TIMER_IRQ_CTRL = 2;                  // unmask timer interrupt
}

void disable_timer_interrupt()
{
    write_sysreg(cntp_ctl_el0, 0);              // disable cpu timer
    *CORE0_TIMER_IRQ_CTRL = 0;                  // mask timer interrupt
}

void set_expired_time(uint64_t duration)
{
    unsigned long frq = read_sysreg(cntfrq_el0);
    write_sysreg(cntp_tval_el0, frq * duration); // set expired time
}

uint64_t get_current_time()
{
    /* You can get the seconds after booting 
       from the count of the timer(cntpct_el0) and the frequency of the timer(cntfrq_el0).
    */
    uint64_t frq = read_sysreg(cntfrq_el0);
    uint64_t current_count = read_sysreg(cntpct_el0);
    return (uint64_t)(current_count / frq);    
}

/* Advanced part */

typedef void (*timer_event_call_back)(char *);

static LIST_HEAD(timer_event_queue);

struct timer_event {
    struct list_head head;
    uint64_t command_time;
    uint64_t expired_time;
    timer_event_call_back callback;
    char message[32];
};

static struct timer_event *timer_event_create(timer_event_call_back cb, char *msg, uint64_t duration)
{
    struct timer_event *event = chunk_alloc(sizeof(struct timer_event));
    INIT_LIST_HEAD(&event->head);
    event->command_time = get_current_time();
    event->expired_time = event->command_time + duration;
    event->callback = cb;
    my_strncpy(event->message, msg, 32);
    return event;
}

static void timer_event_queue_add(struct timer_event *event)
{
    struct list_head *curr = timer_event_queue.next;

    while (curr != &timer_event_queue && (((struct timer_event *) curr)-> expired_time <= event->expired_time))
        curr = curr->next;
    list_add(&event->head, curr->prev, curr);

    // if the new event is inserted to be the first node, update the timer expired time
    if (node_is_first((struct list_head *)event, &timer_event_queue)) {
        set_expired_time(event->expired_time - get_current_time());
        enable_timer_interrupt();
    }
}

static void print_message(char *msg)
{
    uart_send_string("timer event message: ");
    uart_send_string(msg);
    uart_send_string("\r\n");
}


/* Create an API that user program can use */
static void timer_add_event(timer_event_call_back cb, char *msg, uint64_t duration)
{
    struct timer_event *event = timer_event_create(cb, msg, duration);
    timer_event_queue_add(event);
}

/* An example of API use */
void add_timeout_event(char *data, uint64_t duaration)
{
    timer_add_event(print_message, data, duaration);
}

static void delay(uint64_t time)
{
    while (time--) {
        asm volatile ("nop");
    }
}

/*
 * Core timer IRQ:
 * 1. Run the first event of the list
 * 2. Remove the first event
 * 3. Get expired time of next event
 * 4. Update expired time of the core timer
 */

void timer_interrupt_handler()
{
    uart_send_string("In timer interrupt handler: \r\n");

    struct timer_event *event = (struct timer_event *) timer_event_queue.next;
    
    list_del((struct list_head *)event);

    // add delay time
    delay((uint64_t)1 << 22);
    event->callback(event->message);

    uart_send_string("Command time: ");
    uart_hex(event->command_time);
    uart_send_string(" s\r\n");

    uart_send_string("Current time: ");
    uart_hex(get_current_time());
    uart_send_string(" s\r\n");

    chunk_free((char *)event);

    if (!list_is_empty(&timer_event_queue)) {
        struct timer_event *first = (struct timer_event *) timer_event_queue.next;
        set_expired_time(first->expired_time - get_current_time());
        enable_timer_interrupt();
    } else { // timer_queue is empty, need to set a bigger time.
        set_expired_time(1000 + get_current_time());
    }
}