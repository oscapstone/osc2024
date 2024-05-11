#include "uart.h"
#include "allocator.h"
#include "schedule.h"
#include "timer.h"

#define MAX_TIMER_HEAP_SIZE 256

timer_heap *timer_hp = 0;

void timer_heap_init()
{
    timer_hp = create_timer_heap(MAX_TIMER_HEAP_SIZE);
}

void set_min_expire()
{
    unsigned long long min_expire = timer_hp->arr[0].expire;
    unsigned long long cntpct_el0 = 0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0)); // get timer’s current count.
    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0)); // get timer's frequency

    unsigned long long remain = min_expire - cntpct_el0;
    asm volatile(
        "msr cntp_tval_el0, %0\n" ::"r"(remain)); // set expired time
}

void add_timer(timer t)
{
    core_timer_disable();
    timer_heap_insert(timer_hp, t); // insert timer to min heap
    set_min_expire();               // find the min expire timer and set it to hareware timer
    core_timer_enable();
}

void re_shedule(void *data, int executed_time)
{
    get_current_task()->need_sched = 1;

    unsigned long long cntpct_el0 = 0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0)); // get timer’s current count.
    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0)); // get timer's frequency

    timer temp;
    temp.callback = re_shedule;
    temp.expire = cntpct_el0 + (cntfrq_el0 >> 5);
    add_timer(temp);
}

void core_timer_enable()
{
    asm volatile(
        "mov x0, 2\n"
        "ldr x1, =0x40000040\n"
        "str w0, [x1]\n"); // unmask timer interrupt
}

void core_timer_disable()
{
    asm volatile(
        "mov x0, 0\n"
        "ldr x1, =0x40000040\n"
        "str w0, [x1]\n"); // mask timer interrupt
}
