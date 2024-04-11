#include "timer.h"
#include "alloc.h"
#include "string.h"
#include "uart.h"

typedef struct __timer_t {
    void (*func)(void *);
    void *arg;
    int time;
    struct __timer_t *next;
} timer_t;

static timer_t *head = 0;

void timer_enable_interrupt()
{
    asm volatile("mov x0, 1;"
                 "msr cntp_ctl_el0, x0;" // Enable
                 "mrs x0, cntfrq_el0;"
                 "msr cntp_tval_el0, x0;" // Set expired time
                 "mov x0, 2;"
                 "ldr x1, =0x40000040;"
                 "str w0, [x1];"); // Unmask timer interrupt
}

void timer_disable_interrupt()
{
    asm volatile("mov x0, 0;"
                 "ldr x1, =0x40000040;"
                 "str w0, [x1];"); // Mask timer interrupt
}

void timer_irq_handler()
{
    // Set up 1 second core timer interrupt
    asm volatile("mrs x0, cntfrq_el0;"
                 "msr cntp_tval_el0, x0;");

    // Check the timer queue
    while (head != 0 && timer_get_uptime() >= head->time) {
        head->func(head->arg); // Execute the callback function
        head = head->next;     // Delete the node FIXME: free the node
    }

    timer_enable_interrupt();
}

uint64_t timer_get_uptime()
{
    uint64_t cntpct_el0 = 0;
    uint64_t cntfrq_el0 = 0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
    return cntpct_el0 / cntfrq_el0;
}

void timer_add(void (*callback)(void *), void *arg, int after)
{
    // Insert the new timer node into the linked list (sorted by time)

    timer_t *timer = (timer_t *)simple_malloc(sizeof(timer_t));
    timer->func = callback;
    timer->arg = arg;
    timer->time = timer_get_uptime() + after;
    timer->next = 0;

    if (head == 0 || timer->time < head->time) {
        // Insert at the beginning of the list
        timer->next = head;
        head = timer;
        return;
    }

    timer_t *current = head;
    while (current->next != 0 && current->next->time <= timer->time)
        current = current->next;
    timer->next = current->next;
    current->next = timer;
}

void set_timeout(const char *message, int after)
{
    timer_add((void (*)(void *))uart_puts, (void *)message, after);
}