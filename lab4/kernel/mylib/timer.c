#include "timer.h"

struct list_head timer_event_list;


/*
    Enable timer interrupt:
    1.  cntp_ctl_el0 to 1 (bit 0)
    2.  CORE0_TIMER_IRQ_CTRL set to 2
    3.  enable cpu core interrupt

*/
void core_timer_enable(){
    // according to given assembly    

    // set register cntp_ctl_el1 to 1
    asm volatile("msr cntp_ctl_el0, %0"::"r"(1));   // enable timer
        
}

void core_timer_disable(){
    // set register cntp_ctl_el0 to zero
    asm volatile("msr cntp_ctl_el0, %0"::"r"(0)); // disable
    core_timer_interrupt_disable();
}

void core_timer_interrupt_enable(){
    
    asm volatile (
        "mov x2, 2\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t");
    
    //*CORE0_TIMER_IRQ_CTRL = 2;
}
void core_timer_interrupt_disable(){
    
    asm volatile (
        "mov x2, 0\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t");
    
   //*CORE0_TIMER_IRQ_CTRL = 0;
}


void set_core_timer_interrupt(unsigned long long sec){
    asm volatile("msr cntp_cval_el0, %0"::"r"(sec));  // set expired time
}

void set_core_timer_interrupt_first() {


    timer_event_t *first = list_entry(timer_event_list.next, timer_event_t, listhead);

    asm volatile("msr cntp_cval_el0, %0"::"r"(first->expire_time));

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
    list_for_each(listptr, &timer_event_list) {
        timer_event_t *now;
        now = list_entry(listptr, timer_event_t, listhead);
        uart_async_printf("address: %x, expire time:%d : %s\n",&now->listhead, now->expire_time, now->args);
    }
}

void add_timer(timer_callback_t callback, void* arg, unsigned long long expire_time){ 

   // disable_interrupt();
    struct timer_event *entry = (struct timer_event*)simple_malloc(sizeof(struct timer_event));
    
    //uart_printf("size : %d\n",n);
    entry->args = (char *)simple_malloc(32);
    strcpy(entry->args, (char *)arg);

    entry->callback = callback;
    entry->expire_time = expire_time;
    entry->listhead.next = &(entry->listhead);
    entry->listhead.prev = &(entry->listhead);
    //enable_interrupt();
    
    //uart_printf("args size : %d : '%s'\n",strlen(entry->args), entry->args);
    //uart_printf("arg size : %d : '%s'\n",strlen(entry->args), entry->args);
   


    disable_interrupt();

    if(list_empty(&timer_event_list)) {
        list_add_tail(&entry->listhead, &timer_event_list);
    }
    else {
        add_Node_timer(&timer_event_list, entry);
    }

    set_core_timer_interrupt_first();
    enable_interrupt();
    core_timer_interrupt_enable();
    
    
}



void pop_timer() {


    timer_event_t *first = list_entry(timer_event_list.next, timer_event_t, listhead);
   
    disable_interrupt();
    list_del(&first->listhead);     // del first node
    enable_interrupt();
    
    first->callback(first->args);    // exec callback func

    // check empty
    disable_interrupt();
    
    if(list_empty(&timer_event_list)) {
        core_timer_interrupt_disable_alternative();   // turn off timer interrupt
    }
    else {
        set_core_timer_interrupt_first();
    }
    enable_interrupt();
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
    set_core_timer_interrupt(10000 * get_clock_freq() + get_clock_tick());
}

void core_timer_handler() {
    if(list_empty(&timer_event_list)) {
        core_timer_interrupt_disable_alternative();
        return;
    }
    //uart_puts("core_timer_handler...\n");
    pop_timer();
}

void two_second_alert(const char *str) {
    //int n = strlen(str);
    //uart_printf("tsa : size : %d\n",n);
    uart_printf("'%s': seconds after booting : %d\n", str, get_clock_time());
    add_timer(two_second_alert, str, (unsigned long long)((unsigned long long)2 * get_clock_freq() + get_clock_tick()));
}
