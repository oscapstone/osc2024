#include "uart.h"

#define BUFFER_SIZE 1024
#define MAX_TIMER 10

typedef void (*timer_callback_t)(char* data);

struct timer{
    timer_callback_t callback;
    char data[BUFFER_SIZE];
    unsigned long expires;
};

struct timer timers[MAX_TIMER];

unsigned long get_current_time(){//and set next
    unsigned long cntfrq, cntpct;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    //asm volatile ("msr cntp_tval_el0, %0" : : "r" (cntfrq));//*2
    asm volatile ("mrs %0, cntpct_el0" : "=r" (cntpct));
    cntpct /= cntfrq;
    return cntpct;
}

void set_timer_interrupt(unsigned long second){//and set next
    unsigned long ctl = 1;
    asm volatile ("msr cntp_ctl_el0, %0" : : "r" (ctl));
    unsigned long cntfrq;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (cntfrq));
    asm volatile ("msr cntp_tval_el0, %0" : : "r" (cntfrq * second));
    asm volatile ("mov x0, 2");
    asm volatile ("ldr x1, =0x40000040");
    asm volatile ("str w0, [x1]");
}


void add_timer(timer_callback_t callback, char* data, unsigned long after){
    //uart_int(after);
    asm volatile("msr DAIFSet, 0xf");
    int i;
    int allocated = 0;

    unsigned long cur_time = get_current_time();
    unsigned long print_time = cur_time + after;
    for(i=0; i<MAX_TIMER; i++){
        if(timers[i].expires == 0){
            timers[i].expires = print_time;
            int j = 0;
            while(*data != '\0'){
                timers[i].data[j] = *data;
                data++;
                j++;
            }
            timers[i].data[j] = '\0';
            //uart_puts("timer data: ");
            //uart_puts(timers[i].data[j]);
            timers[i].callback = callback;
            allocated = 1;
            break;
        }
    }

    if(allocated == 0){
        uart_puts("Timer Busy\n");
    }

    int new_irq = 1;
    unsigned long min_time = print_time;
    for(int i=0; i<MAX_TIMER; i++){
        if(timers[i].expires < min_time && timers[i].expires > 0){
            new_irq = 0;
            break;
        }
    }

    if(new_irq){
        // uart_puts("New Interrupt Set in timer ");
        // uart_int(i);
        // uart_puts("\n");
        set_timer_interrupt(print_time - cur_time);
        //asm volatile ("msr cntp_tval_el0, %0" : : "r" (cntfrq));
    }
    uart_puts("\rSeconds to print: ");
    uart_int(print_time);
    uart_puts("\n");
    asm volatile("msr DAIFClr, 0xf");
}


void setTimeout_callback(char* data) {
    // Convert data back to the appropriate type and print the message
    uart_puts(data);
    uart_puts("\n");
    uart_send('\r');
    uart_puts("# ");
}

void setTimeout_cmd(){
    uart_puts("\rMESSAGE: ");
    char in_char;
    char message[100];
    int idx = 0;
    while(1){
        in_char = uart_getc();
        uart_send(in_char);
        if(in_char == '\n'){
            message[idx] = '\0';
            idx = 0;
            break;
        }
        else{
            message[idx] = in_char;
            idx++;
        }
    }
    uart_puts("\rSECONDS: ");
    idx = 0;
    char countDown[100];
    while(1){
        in_char = uart_getc();
        uart_send(in_char);
        if(in_char == '\n'){
            countDown[idx] = '\0';
            idx = 0;
            break;
        }
        else{
            countDown[idx] = in_char;
            idx++;
        }
    }
    unsigned long cur_time, print_time;
    unsigned long wait = str2int(countDown);
    if(wait <= 0){
        uart_puts("\rINVALID TIME\n");
        return;
    }
    
    add_timer(setTimeout_callback, message, wait);
}






void disable_core_timer() {
    // disable timer
    unsigned long ctl = 0;
    asm volatile ("msr cntp_ctl_el0, %0" : : "r" (ctl));

    // mask timer interrupt
    asm volatile ("mov x0, 0");
    asm volatile ("ldr x1, =0x40000040");
    asm volatile ("str w0, [x1]");
}

void timer_handler() {
    asm volatile("msr DAIFSet, 0xf");
    unsigned long cur_time = get_current_time();
    unsigned long next = 9999;
    for(int i=0;i<MAX_TIMER;i++){
        if(timers[i].expires == cur_time){
            uart_puts("\n[TIMER] ");
            uart_int(cur_time);
            uart_puts(" : ");
            timers[i].callback(timers[i].data);
            timers[i].expires = 0;
        }
        else if(timers[i].expires > cur_time){
            if(next > timers[i].expires)
                next = timers[i].expires;
        }
    }
    disable_core_timer();
    if(next != 9999){
        set_timer_interrupt(next - cur_time);
        //uart_puts("resetted another timer\n");
    }
    asm volatile("msr DAIFClr, 0xf");
}