#include <kernel/bsp_port/irq.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/timer.h>

typedef struct timer_t {
    void (*func)(void *);
    void *arg;
    int time;
    struct timer_t *next;
} timer_t;

static timer_t *head = (timer_t *)0;

void set_timeout(void (*callback)(void *), void *arg, int delay) {
    timer_t *timer = (timer_t *)simple_malloc(sizeof(timer_t));
#ifdef DEBUG
    print_string("\n[set_timeout] timer_t allocated at ");
    print_h((unsigned long)timer);
    print_string("\n");
    print_string("[set_timeout] head: ");
    print_h((unsigned long)head);
    print_string("\n");
#endif /* ifdef DEBUG */

    timer->func = callback;
    timer->arg = arg;
    timer->time = timer_get_uptime() + delay;
    timer->next = 0;

    if (head == 0 || timer->time < head->time) {
        // Insert at the beginning of the list
        timer->next = head;
        head = timer;
        core_timer_enable();
        return;
    }

    timer_t *current = head;
    while (current->next != 0 && current->next->time <= timer->time)
        current = current->next;
    timer->next = current->next;
    current->next = timer;
}

void timer_irq_handler() {
    // Check the timer queue
    while (head != 0 && timer_get_uptime() >= head->time) {
        head->func(head->arg);
        head = head->next;  // TODO: free the node
    }

    if (head != 0) core_timer_enable();
}
