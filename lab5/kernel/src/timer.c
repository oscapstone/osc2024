#include "timer.h"
#include "mini_uart.h"
#include "utils.h"
#include "list.h"
#include "memory.h"

#define XSTR(s) STR(s)
#define STR(s) #s

/* Empty head -> timer -> timer .... */
struct list_head* timer_list; 

void timer_list_init() {
    timer_list = malloc(sizeof(timer_event_t));
    INIT_LIST_HEAD(timer_list);
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
}

void core_timer_enable() {
    __asm__ __volatile__(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t" // cntp_ctl_el0[0]: enable, Control register for the EL1 physical timer.
                                   // cntp_tval_el0: Holds the timer value for the EL1 physical timer
        "mov x2, 2\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t"         // QA7_rev3.4.pdf: Core0 Timer IRQ allows Non-secure physical timer(nCNTPNSIRQ)
    );
}

void core_timer_disable() {
    __asm__ __volatile__(
        "mov x2, 0\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t"         // QA7_rev3.4.pdf: Mask all timer interrupt
    );
}

// set timer interrupt time to [expired_time] seconds after now (relatively)
void set_timer_interrupt(unsigned long long seconds) {
    unsigned long long freq;
    unsigned long long ticks;

    __asm__ __volatile__(
        "mrs %0, cntfrq_el0\n\t" : "=r"(freq)
    );
    ticks = freq * seconds;
    __asm__ __volatile__(
        "msr cntp_tval_el0, %0\n\t" : "=r"(ticks)
    );
}

// directly set timer interrupt time to a cpu tick  (directly)
void set_timer_interrupt_by_tick(unsigned long long tick) {
    __asm__ __volatile__(
        "msr cntp_cval_el0, %0\n\t"  //cntp_cval_el0 -> absolute timer
        :"=r" (tick)
    );
}

// get cpu tick add some second
unsigned long long get_cpu_tick_plus_s(unsigned long long seconds){
    unsigned long long cntpct_el0=0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t": "=r"(cntpct_el0)); // tick anchor
    unsigned long long cntfrq_el0=0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); // tick frequency
    return (cntpct_el0 + cntfrq_el0*seconds);
}

// get cpu tick add some tick
uint64_t get_tick_plus_t(uint64_t tick) {
    uint64_t cntpct_el0 = 0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t" : "=r"(cntpct_el0)); // tick auchor
    //DEBUG("cntpct_el0: %d\r\n", cntpct_el0);
    return (cntpct_el0 + tick);
}

void set_alert_2S(char* str) {
    unsigned long long cntpct_el0;
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t": "=r"(cntpct_el0)); // tick anchor
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); // tick frequency
    
    uart_puts("\r\n[Start Alert]\r\n");
    
    // for(int i=0;i<1000000000;i++){} 
    
    char buf[VSPRINT_MAX_BUF_SIZE];
    sprintf(buf, "[Interrupt][el1_irq][%s] %d seconds after booting\n", str, cntpct_el0/cntfrq_el0);
    uart_puts(buf);

    unsigned long long interrupt_time = get_cpu_tick_plus_s(2);
    set_timer_interrupt_by_tick(interrupt_time);
    // Add timer
    add_timer(set_alert_2S, 2, str);
}

void add_timer(void *callback, unsigned long long timeout, char* args) {
    timer_event_t* e = malloc(sizeof(timer_event_t));
    
    e->args = malloc(strlen(args) + 1);
    strcpy(e->args, args);

    e->interrupt_time = get_cpu_tick_plus_s(timeout);
    e->callback = callback;

    INIT_LIST_HEAD(&e->listhead);

    /* Add timer to the sorted timer_list */
    struct list_head* ptr;
    list_for_each(ptr, timer_list) {
        if (((timer_event_t*)ptr)->interrupt_time > e->interrupt_time) {
            // Insert the event just before the event with bigger time
            list_add(&e->listhead, ptr->prev);
            break;
        }
    }
    /* If the event interrupt time is biggest, add to the tail of the tiemr_list */
    if (list_is_head(ptr, timer_list)) {
        list_add_tail(&e->listhead, timer_list);
    }

    set_timer_interrupt_by_tick(((timer_event_t*)timer_list->next)->interrupt_time);
}

void add_timer_by_tick(void *callback, uint64_t tick, void *args) {
    // DEBUG("add_timer_by_tick: %d\r\n", tick);
    timer_event_t *e = kmalloc(sizeof(timer_event_t)); // free by timer_event_callback
    // store all the related information in timer_event
    e->args = args;
    e->interrupt_time = get_tick_plus_t(tick);
    // DEBUG("the_timer_event->interrupt_time: %d\r\n", the_timer_event->interrupt_time);
    e->callback = callback;
    INIT_LIST_HEAD(&e->listhead);

    // add the timer_event into timer_event_list (sorted)
    struct list_head *curr;
    list_for_each(curr, timer_list)
    {
        if (((timer_event_t *)curr)->interrupt_time > e->interrupt_time)
        {
            list_add(&e->listhead, curr->prev); // add this timer at the place just before the bigger one (sorted)
            break;
        }
    }
    // if the timer_event is the biggest, run this code block
    if (list_is_head(curr, timer_list)) {
        list_add_tail(&e->listhead, timer_list);
    }
    // set interrupt to first event
    set_timer_interrupt_by_tick(((timer_event_t *)timer_list->next)->interrupt_time);
    core_timer_enable();
}

void core_timer_handler() {
    if (list_empty(timer_list)) {
        set_timer_interrupt(100000); // disable timer interrupt (set a very big value)
        return;
    }
    timer_event_callback((timer_event_t *)timer_list->next); // do callback and set new interrupt
}

void timer_event_callback(timer_event_t * timer_event) {
    list_del_entry((struct list_head*)timer_event); // delete the event in queue
    ((void (*)(char*))timer_event-> callback)(timer_event->args);  // call the event

    // set queue linked list to next time event if it exists
    if(!list_empty(timer_list)) {
        set_timer_interrupt_by_tick(((timer_event_t*)timer_list->next)->interrupt_time);
    } else {
        set_timer_interrupt(10000);  // disable timer interrupt (set a very big value)
    }
}
