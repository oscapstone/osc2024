#include"header/timer.h"
#include "header/malloc.h"
#include "header/utils.h"

// struct list_head* timer_event_list;  // first head has nothing, store timer_event_t after it 
timer *timer_list = 0;

void core_timer_interrupt_enable(){
    asm volatile("mov x1, 1\n\t");//The purpose of this instruction depends on the context of the program. Typically, it's used to prepare data or a control signal for subsequent operations.
    asm volatile("msr cntp_ctl_el0, x1\n\t");//This register often controls various aspects of the timer in ARM architectures, such as enabling or disabling it.
    asm volatile("mov x2, 2\n\t");// prepares a value for subsequent operations.
    asm volatile("ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t");//memory-mapped control register related to the timer's interrupt control.
    asm volatile("str w2, [x1]\n\t");//The value in register w2 (lower 32 bits of x2) is stored into the memory location pointed to by x1. This effectively writes the value 2 to the memory-mapped control register represented by CORE0_TIMER_IRQ_CTRL.
}
void core_timer_interrupt_disable(){
    asm volatile("mov x2, 0\n\t");//The purpose of this operation depends on the context of the program. Register x2 may be used as a general-purpose register to hold data or addresses.
    asm volatile("ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t");// ads the address of a symbol named CORE0_TIMER_IRQ_CTRL into register x1. The symbol likely represents the memory-mapped control register related to the timer's interrupt control. The = operator with ldr instruction is typically used to load an address directly into a register.
    asm volatile("str w2, [x1]\n\t");//tored into the memory location pointed to by x1. This effectively writes the value 0 to the memory-mapped control register represented by CORE0_TIMER_IRQ_CTRL
}
void core_timer_interrupt_disable_alternative() {
    unsigned long long sec = 0xFFFFFFFFFFFFFFFF;
    // unsigned long long sec = 10000;
    asm volatile("msr cntp_cval_el0, %0" ::"r"(sec));
}

unsigned long long get_tick_plus_s(unsigned long long second){
    unsigned long long cntpct_el0=0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t": "=r"(cntpct_el0)); // tick auchor
    unsigned long long cntfrq_el0=0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); // tick frequency
    return (cntpct_el0 + cntfrq_el0*second);
}
void set_core_timer_interrupt(unsigned long long sec){

    //part 2
    // core_timer_interrupt_disable_alternative();

    //part 4
    __asm__ __volatile__(
        "mrs x1, cntfrq_el0\n\t"    // cntfrq_el0 -> frequency of the timer
        "mul x1, x1, %0\n\t"        // cntpct_el0 = cntfrq_el0 * seconds: relative timer to cntfrq_el0
        "msr cntp_tval_el0, x1\n\t" // Set expired time to cntp_tval_el0, which stores time value of EL1 physical timer.
    :"=r" (sec));
}

void timer_list_init(){
    // INIT_LIST_HEAD(timer_event_list);
    timer_list = 0;
}

void add_timer(void* callback, unsigned long long timeout, char* args){
    // create timer node
    timer* event = simple_malloc(sizeof(timer));
    // storing
    event->args = simple_malloc(strlen(args)+1);
    strcpy(event -> args,args); 
    event->interrupt_time = timeout;
    event->callback = callback;
    // add the event into timer_list (sorted)
    timer* cur = timer_list;
    asm volatile("msr DAIFSet, 0xf");
    // insert while list empty or the smallest
    if(timer_list == 0 || timer_list->interrupt_time > event->interrupt_time){
        event -> next = timer_list;
        event -> prev = 0;
        if(timer_list != 0){
            timer_list -> prev = event;
        }
        timer_list = event;
        set_core_timer_interrupt(timer_list->interrupt_time);
    }
    else {
        while (cur->next != 0 && cur->next->interrupt_time < event->interrupt_time) {
            cur = cur->next;
        }
        event->next = cur -> next;
        event->prev = cur;
        if(cur->next != 0){
            cur->next->prev = event;
        }
        cur->next = event;
    }
    asm volatile("msr DAIFClr, 0xf");
}

void poptimer(){
    asm volatile("msr DAIFSet, 0xf");
    while (timer_list) {
        timer *cur = timer_list;
        ((void (*)())cur->callback)(cur->args);
        timer_list = timer_list->next;
        cur = timer_list;
        timer_list -> prev = 0;
        if(timer_list == 0)
        {
            core_timer_interrupt_disable_alternative();// disable timer interrupt (set a very big value)
        }
        else
        {
            set_core_timer_interrupt(cur->interrupt_time);  
        }
    }   
    asm volatile("msr DAIFClr, 0xf");
}

void core_timer_handler(){
    if (timer_list == 0)
    {
        core_timer_interrupt_disable_alternative(); // disable timer interrupt (set a very big value)
        return;
    }
    poptimer(); // do callback and set new interrupt
    
}