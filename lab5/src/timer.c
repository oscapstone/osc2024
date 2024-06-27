#include "../include/timer.h"
#include "../include/timer_utils.h"
#include "../include/list.h"
#include "../include/mem_utils.h"
#include "../include/string_utils.h"
#include "../include/mini_uart.h"
#include "../include/peripherals/mini_uart.h"
#include "../include/sched.h"
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

void set_expired_time(uint64_t cycles)
{
    write_sysreg(cntp_tval_el0, cycles);  // set expired time
}

uint64_t get_current_time()
{
    /* You can get the cycles after booting 
       from the count of the timer(cntpct_el0).
    */
    uint64_t current_count = read_sysreg(cntpct_el0);
    return current_count;
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
    uint32_t is_periodic;
};

static struct timer_event *timer_event_create(timer_event_call_back cb, char *msg, uint64_t cycles, uint32_t is_periodic)
{
    struct timer_event *event = chunk_alloc(sizeof(struct timer_event));
    INIT_LIST_HEAD(&event->head);
    event->command_time = get_current_time();
    event->expired_time = event->command_time + cycles;
    event->callback = cb;
    event->is_periodic = is_periodic;
    if (msg) 
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
static void timer_add_event(timer_event_call_back cb, char *msg, uint64_t cycles, uint32_t is_periodic)
{
    struct timer_event *event = timer_event_create(cb, msg, cycles, is_periodic);
    timer_event_queue_add(event);
}

/* An example of API use */
void add_timeout_event(char *data, uint64_t cycles, uint32_t is_periodic)
{
    if (data)
        timer_add_event(print_message, data, cycles, is_periodic);
    else
        timer_add_event(timer_tick, data, cycles, is_periodic);
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
    // uart_send_string("In timer interrupt handler: \r\n");

    struct timer_event *event = (struct timer_event *) timer_event_queue.next;
    
    list_del((struct list_head *)event);

    if (event->is_periodic) {
        uint64_t cycles = read_sysreg(cntfrq_el0) >> 5;
        add_timeout_event(NULL, cycles, event->is_periodic);
    }

    event->callback(event->message);

    // uart_send_string("Command time: ");
    // uart_hex(event->command_time);
    // uart_send_string(" s\r\n");

    // uart_send_string("Current time: ");
    // uart_hex(get_current_time());
    // uart_send_string(" s\r\n");

    chunk_free((char *)event);
    // printf("free timer event!\n");

    if (!list_is_empty(&timer_event_queue)) {
        struct timer_event *first = (struct timer_event *) timer_event_queue.next;
        set_expired_time(first->expired_time - get_current_time());
        enable_timer_interrupt();
    } else { // timer_queue is empty, need to set a bigger time.
        set_expired_time(get_current_time() + read_sysreg(cntfrq_el0) >> 5);
    }
}