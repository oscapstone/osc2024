#include "timer.h"
#include "uart.h"
#include "uint.h"
#include "irq.h"
#include "malloc.h"
#include "registers.h"
#include "string.h"
#include "utils.h"


struct list_head *timer_event_list;


/*
    Enable timer interrupt:
    1.  cntp_ctl_el0 to 1 (bit 0)
    2.  CORE0_TIMER_IRQ_CTRL set to 2
    3.  enable cpu core interrupt

*/

void timer_list_init()
{
    cpu_timer_enable();
    timer_event_list = malloc(sizeof(list_head_t));
    INIT_LIST_HEAD(timer_event_list);
}

void cpu_timer_enable() {
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" ::"r"(tmp));
}

void core_timer_enable(){
    // according to given assembly    

    // set register cntp_ctl_el1 to 1
    asm volatile("msr cntp_ctl_el0, %0"::"r"(1));   // enable timer


    // exception 04
    core_timer_interrupt_enable();

   
}

void core_timer_disable(){
    // set register cntp_ctl_el0 to zero
    // asm volatile("msr cntp_ctl_el0, %0"::"r"(0)); // disable
    core_timer_interrupt_disable();
}

void core_timer_interrupt_enable(){
    
    asm volatile (
        "mov x2, 2\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t" // unmask timer interrupt
    :::"x1","x2");
    
    //*CORE0_TIMER_IRQ_CTRL = 2;
}
void core_timer_interrupt_disable(){
    
    __asm__ __volatile__(
        "mov x2, 0\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t" // unmask timer interrupt
    :::"x1","x2");
    
}


// relative time from now (tval)
void set_core_timer_interrupt(unsigned long long expired_time)
{
    __asm__ __volatile__(
        "mrs x1, cntfrq_el0\n\t" 
        "mul x1, x1, %0\n\t"
        "msr cntp_tval_el0, x1\n\t" 
        ::"r"(expired_time)
        : "x1");
}

// absolute time (cval)
void set_core_timer_interrupt_by_tick(unsigned long long tick)
{
    __asm__ __volatile__(
        "msr cntp_cval_el0, %0\n\t" 
        ::"r"(tick));
}


void set_core_timer_interrupt_first() {


    timer_event_t *first = list_entry(timer_event_list->next, timer_event_t, listhead);

    asm volatile("msr cntp_cval_el0, %0"::"r"(first->expire_time));

}

void timer_event_callback(timer_event_t *timer_event) {
    ((void (*)(char *))timer_event->callback)(timer_event->args); // call the callback store in event
    list_del((struct list_head *)timer_event);              // delete the event
    free(timer_event->args);                                      // free the arg space
    free(timer_event);

    // set interrupt to next time_event if existing
    if (!list_empty(timer_event_list))
        set_core_timer_interrupt_by_tick(((timer_event_t *)timer_event_list->next)->expire_time);
    else
        set_core_timer_interrupt(10000); // alternative disable (large value)
}

void add_Node_timer(list_head_t *head, timer_event_t *entry){
  
    struct list_head *listptr;
    
    list_for_each(listptr, head) {
        timer_event_t *now;
        now = list_entry(listptr, timer_event_t, listhead);
        if(entry->expire_time < now->expire_time) {
            list_insert(&entry->listhead, listptr->prev, listptr);
            return;
        }
    }
    list_add_tail(&entry->listhead, head);
  
}

void print_timer(){
    uart_async_printf("Now time event list :\n");
    list_head_t *listptr;
    list_for_each(listptr, timer_event_list) {
        timer_event_t *now;
        now = list_entry(listptr, timer_event_t, listhead);
        uart_async_printf("address: %x, expire time:%d : %s\n",&now->listhead, now->expire_time, now->args);
    }
}

void add_timer(timer_callback_t callback, void* arg, unsigned long long expire_time, int bytick){ 

   // disable_interrupt();
    struct timer_event *entry = malloc(sizeof(struct timer_event));
    
    //uart_printf("size : %d\n",n);
    entry->args = malloc(strlen(arg) + 1);
    strcpy(entry->args, (char *)arg);

    // argument expire time is tick or second
    if (bytick == 0)
        entry->expire_time = get_tick_plus_s(expire_time); // store interrupt time into timer_event
    else
        entry->expire_time = get_tick_plus_s(0) + expire_time;

    entry->callback = callback;
    entry->listhead.next = &(entry->listhead);
    entry->listhead.prev = &(entry->listhead);

   


    lock();

    if(list_empty(timer_event_list)) {
        list_add_tail(&entry->listhead, timer_event_list);
    }
    else {
        add_Node_timer(timer_event_list, entry);
    }

    set_core_timer_interrupt_first();

    unlock();
    
    
}



void pop_timer() {


    timer_event_t *first = list_entry(timer_event_list->next, timer_event_t, listhead);
   
    disable_interrupt();
    list_del(&first->listhead);     // del first node
    enable_interrupt();
    
    first->callback(first->args);    // exec callback func

    // check empty
    disable_interrupt();
    
    if(list_empty(timer_event_list)) {
        core_timer_interrupt_disable_alternative();   // turn off timer interrupt
    }
    else {
        set_core_timer_interrupt_first();
    }
    enable_interrupt();
}


unsigned long long get_tick_plus_s(unsigned long long second)
{
    return (get_clock_tick() + get_clock_freq() * second);
}

unsigned long long get_clock_tick()
{
    unsigned long long cntpct_el0;
    __asm__ __volatile__(
        "mrs %0, cntpct_el0\n\t"
        : "=r"(cntpct_el0)); // tick now

    return cntpct_el0;
}

unsigned long long get_clock_freq()
{
    unsigned long long cntfrq_el0;
    __asm__ __volatile__(
        "mrs %0, cntfrq_el0\n\t"
        : "=r"(cntfrq_el0)); // tick frequency
    return cntfrq_el0;
}

unsigned long long get_clock_time()
{
    return get_clock_tick() / get_clock_freq();
}


void core_timer_interrupt_disable_alternative() {
    set_core_timer_interrupt(10000);
}

void core_timer_handler() {
    lock();
    if(list_empty(timer_event_list)) {
        set_core_timer_interrupt(10000);
        unlock();
        return;
    }
    timer_event_callback((timer_event_t *)timer_event_list->next); // do callback and set new interrupt
    //uart_puts("core_timer_handler...\n");
    unlock();
}

void two_second_alert(const char *str) {
    //int n = strlen(str);
    //uart_printf("tsa : size : %d\n",n);
    uart_printf("'%s': seconds after booting : %d\n", str, get_clock_time());
    add_timer(two_second_alert, str, 2, 0);
}
int timer_list_size()
{
    int r = 0;
    struct list_head *curr;
    list_for_each(curr, timer_event_list) {
        r++;
    }
    return r;
}