#include "time_c.h"
#include "uart.h"
#include "interrupt.h"
#include "printf.h"
#include "utils.h"
#include "allocator.h"

timer_info *timer_head = NULL;


void enqueue_timer(timer_info *newTimer){

    if(!timer_head){
        newTimer->next = timer_head;
        newTimer->prev = NULL;
        timer_head = newTimer;
    }
    else{
        if(newTimer->executeTime < timer_head->executeTime){
            newTimer->next = timer_head;
            newTimer->prev = NULL;
            timer_head = newTimer;
        }
        else if(newTimer->executeTime >= timer_head->executeTime){
            timer_info* current = timer_head;

            while(current->next && newTimer->executeTime >= current->next->executeTime){
                current = current->next;
            }
            newTimer->next = current->next;
            newTimer->prev = current;
            if (current->next) {
                current->next->prev = newTimer;
            }
            current->next = newTimer;
        }
    }

}

void add_timer(char* msg, unsigned long long wait){
    unsigned long long cntfrq_el0 = 0;//The base frequency
	asm volatile("mrs %0,cntfrq_el0":"=r"(cntfrq_el0));
    wait = cntfrq_el0*wait;// wait 2 seconds
    unsigned long long current = 0;
    asm volatile("mrs %0,cntpct_el0":"=r"(current));

	// asm volatile ("msr cntp_tval_el0, %0"::"r"(wait));//set new timer
    // unsigned long long cntp_cval = 0;//The base frequency
	// asm volatile("mrs %0,cntp_cval_el0":"=r"(cntp_cval));

    timer_info* timer = simple_malloc(sizeof(timer_info));
    timer->msg = msg;
    timer->executeTime = current+wait;
    enqueue_timer(timer);
}

void setTimeout(char* message, unsigned long long wait){
    // timer_head->msg = msg;
    // unsigned long long cntfrq_el0 = 0;//The base frequency
	// asm volatile("mrs %0,cntfrq_el0":"=r"(cntfrq_el0));
    // wait = cntfrq_el0*wait;// wait 2 seconds
	// asm volatile ("msr cntp_tval_el0, %0"::"r"(wait));//set new timer

    // delay(1);
    add_timer(message, wait);
    asm volatile ("msr cntp_cval_el0, %0"::"r"(timer_head->executeTime));//set new timer

    asm volatile("bl core_timer_irq_enable");
    // enable_interrupt();

    if(timer_head){
        asm volatile ("bl core_timer_irq_enable");
    }

    // while(!(utils_string_compare(timer_head->msg,"")));
}

void timer_init(){
    // unsigned long long wait;
    // timer_head->msg = "The computer has finished booting up";
    // asm volatile("bl core_timer_irq_enable");
    // unsigned long long cntfrq_el0 = 0;//The base frequency
	// asm volatile("mrs %0,cntfrq_el0":"=r"(cntfrq_el0));
    // wait = cntfrq_el0*2;// wait 2 seconds
	// asm volatile ("msr cntp_tval_el0, %0"::"r"(wait));//set new timer
    setTimeout("The computer has finished booting up", 2);
    enable_interrupt();
}