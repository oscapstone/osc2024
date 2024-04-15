#include "timer.h"

timer_t t_queue[10];

void init_time_queue(){
    for (int i = 0; i < 10; i++){
        t_queue[i].valid = 1;
        t_queue[i].with_arg = 0;
    }
}

void enable_core_timer() { 
    asm volatile(
        "mov    x0, 1;"             
        "msr    cntp_ctl_el0, x0;"  // enable
        "mrs    x0, cntfrq_el0;"    
        "msr    cntp_tval_el0, x0;"  // set expired time
        "mov    x0, 2;"
        "ldr    x1, =0x40000040;"
        "str    w0, [x1];"          // unmask timer interrupt
    );
}

void disable_core_timer() { //ok
    asm volatile(
        "mov    x0, 0;"             
        "msr    cntp_ctl_el0, x0;"  // disable
        "mrs    x0, cntfrq_el0;"    
        "msr    cntp_tval_el0, x0;"  // set expired time
        "mov    x0, 0;"
        "ldr    x1, =0x40000040;"
        "str    w0, [x1];"          // unmask timer interrupt
    );

}

uint64_t get_core_freq() { //ok
    uint64_t freq;
    asm volatile("mrs %0, cntfrq_el0;" :"=r"(freq));
    return freq;
}

uint64_t get_core_count() { //ok
    uint64_t count;
    asm volatile("mrs %0, cntpct_el0;" :"=r"(count));
    return count;
}

uint32_t get_current_time(){
    uint64_t freq = get_core_freq();
    uint64_t count = get_core_count();

    uint32_t current_time = count / freq;

    return current_time;
}

void add_timer(void (*callback)(), int expire_time){

    disable_interrupt();
    uint32_t timer_id = 0;

    for ( ; timer_id < 10; timer_id++){
        if (t_queue[timer_id].valid) {
            t_queue[timer_id].valid = 0;
            t_queue[timer_id].func = callback;
            t_queue[timer_id].with_arg = 0;
            t_queue[timer_id].expire_time = expire_time;

            break;
        }
    }
    enable_interrupt();
}

void add_timer_arg(void (*callback)(char*), char* arg, int expire_time){

    disable_interrupt();
    uint32_t timer_id = 0;
    
    for ( ; timer_id < 10; timer_id++){
        if (t_queue[timer_id].valid) {
            t_queue[timer_id].valid = 0;
            t_queue[timer_id].func_arg = callback;
            t_queue[timer_id].arg = arg;
            t_queue[timer_id].with_arg = 1;
            t_queue[timer_id].expire_time = expire_time;

            break;
        }
    }   
    enable_interrupt();
}

void set_timeout(char* msg, uint32_t expire_time){
    void (*callback)(char*);
    callback = async_uart_puts;

    add_timer_arg(callback, msg, expire_time);
}

void sleep(void (*wakeup_func)(), uint32_t expire_time){
    add_timer(wakeup_func, expire_time);
}

void print_current_time() { 
    uint32_t current_time = get_current_time();

    print_str("\nCurrent Time (after booting): ");
    print_hex(current_time);
    print_str("(secs)");
}

void one_sec_pass(){
    // print_str("\nTimer decrease");
    for (int i = 0; i < 10; i++){
        if (!t_queue[i].valid){
            t_queue[i].expire_time--;

            if (t_queue[i].expire_time <= 0){
                print_current_time();
                if (t_queue[i].with_arg){
                    t_queue[i].func_arg(t_queue[i].arg);
                }else{
                    t_queue[i].func();
                }

                t_queue[i].valid = 1;
            }
        }
    }
}

void core_timer_handler(){
    asm volatile(
        "mrs x0, cntfrq_el0;"
        "msr cntp_tval_el0, x0;"
    );

    // print_str("\nhere");

    one_sec_pass();
}

void two_sec_timer_handler(){
    print_current_time();

    asm volatile(
        "mov    x0, 1;"             
        "msr    cntp_ctl_el0, x0;"  // enable
        "mrs    x0, cntfrq_el0;"
        "add    x0, x0, x0;"
        "msr    cntp_tval_el0, x0;"
        "mov    x0, 2;"
        "ldr    x1, =0x40000040;"
        "str    w0, [x1];"          // unmask timer interrupt
    );


}