#include "include/interrupt.h"
#include "include/uart.h"
#include "include/utils.h"

#define CORE0_INTERRUPT_SOURCE	((volatile unsigned int*)(0x40000060))

struct Timer* timer_queue_head = NULL;

int print_boot_timer(){
    uart_puts("Interrupt occurs!\n");
    unsigned long long freq, ticks, next_tick;
    asm volatile(
    "mrs %0, cntfrq_el0;"
    "mrs %1, cntpct_el0;"
        : "=r" (freq), "=r" (ticks)
    );
    // set up the next timer - basic
    // asm volatile (
    //   "msr cntp_tval_el0, %0" 
    //     : 
    //     : "r" (freq * 2)
    // );

    // set up next timer - advanced (with timer queue)
    timer_queue_head = timer_queue_head->next;
    if(timer_queue_head != NULL){
        next_tick = timer_queue_head->trigger_tick;
        uart_hex(next_tick);
        uart_puts("\t");
        uart_hex(ticks);
        asm volatile (
        "msr cntp_tval_el0, %0" 
            : 
            : "r" (next_tick - ticks)
        );
    }
    else{
        // no more queued core time interrupt, disable the interrupt src
        disable_timer_interrupt();
    }
    unsigned int time = (unsigned int)(ticks/freq);
    uart_puts("\n");
    uart_hex(time);
    uart_puts(" seconds elapsed\n");
  return 0;
}


void interrupt_entry(){
    //asm volatile("msr DAIFSet, 0xf");
    // int irq_pending1 = *IRQ_PENDING_1;
    int core0_irq = *CORE0_INTERRUPT_SOURCE;
    int iir = *AUX_MU_IIR;
    if (core0_irq & (1 << 1)){
        print_boot_timer();
    }
    else{
        // P13, Receiver holds valid byte
        if ((iir & 0x06) == 0x04){
            uart_read_handler();
        }
        // P13, Transmit holding register empty
        //else if((iir & 0x06) == 0x02){
        else{
            uart_write_handler();
        }
    }
    return;
}


unsigned long long get_current_tick(){
	unsigned long long cntpct_el0 = 0;
	asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
	return cntpct_el0;
}

void disable_timer_interrupt(){
    *((volatile unsigned int*)CORE0_TIMER_IRQ_CTRL) = (0x0);
}

void enable_timer_interrupt(){
    *((volatile unsigned int*)CORE0_TIMER_IRQ_CTRL) = (0x2);
}

void print_time_out(int delay){
    uart_puts("Timer interrupt occured at 0x");
    uart_hex(delay);
    uart_puts(" seconds");
    // remove timer head
    timer_queue_head = timer_queue_head->next;
}

void set_time_out(char* message, unsigned long long seconds){
    _set_time_out(print_time_out, message, seconds);
}   

void core_timer_init(){
    core_timer_enable();
    disable_timer_interrupt();
}

void print_time(){
    struct Timer* cur = timer_queue_head;
    while(cur != NULL){
        uart_puts("");
        uart_hex(cur->trigger_tick);
        uart_puts("\n");
        cur = cur->next;
    }
}

void _set_time_out(void (*callback_func)(void *), char* message, unsigned long long delay){
	Timer *new_timer = (Timer *)simple_malloc(sizeof(Timer));
    unsigned long long freq;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    unsigned long long delay_tick = delay*(freq);
    new_timer->callback_function = callback_func;
    new_timer->trigger_tick = delay_tick+get_current_tick();
    new_timer->next = NULL;
    uart_puts("\n");
    uart_hex(new_timer->trigger_tick);
    if(timer_queue_head == NULL || new_timer->trigger_tick<timer_queue_head->trigger_tick){
        new_timer->next = timer_queue_head;
        timer_queue_head = new_timer;
		enable_timer_interrupt();
        asm volatile (
        "msr cntp_tval_el0, %0" 
            : 
            : "r" (timer_queue_head->trigger_tick)
        );
        uart_puts("\nset next interrupt time to 0x");
        uart_hex(timer_queue_head->trigger_tick);
        uart_puts(", current tick = 0x");
        uart_hex(get_current_tick());
        uart_puts("\n");
        return;
    }
    struct Timer *iter = timer_queue_head;
    while(iter->next != NULL && iter->next->trigger_tick <= new_timer->trigger_tick){
        iter = iter->next;
    }
    new_timer->next = iter->next;
    iter->next = new_timer;
    return;
}