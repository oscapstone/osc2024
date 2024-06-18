#include "timer.h"

#define MAX_TIMER 20

timer_t* t_queue;

extern thread_t* cur_thread;

void init_time_queue(){

    t_queue = simple_alloc(MAX_TIMER * sizeof(timer_t));

    for (int i = 0; i < MAX_TIMER; i++){
        t_queue[i].valid = 1;
        t_queue[i].with_arg = 0;
    }
}

void set_kernel_timer(){
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
}

// void set_sched_timer(){
//     asm volatile(
//         "msr    cntp_tval_el0, %0;"  // set expired time
//         :: "r"(get_core_freq() >> 5)
//     ); 
// }

void enable_core_timer() { 

    asm volatile(
        "mov    x0, 1;"             
        "msr    cntp_ctl_el0, x0;"  // enable 
        "mov    x0, 2;"
        "ldr    x1, =0x40000040;"
        "str    w0, [x1];"          // unmask timer interrupt
    );

    asm volatile(
        "msr    cntp_tval_el0, %0;"  // set expired time
        :: "r"(get_core_freq() >> 5)
    );

    // asm volatile(
    //     "mov    x0, 1;"             
    //     "msr    cntp_ctl_el0, x0;"  // disable
    //     "mrs    x0, cntfrq_el0;"    
    //     "msr    cntp_tval_el0, x0;"  // set expired time
    //     "mov    x0, 2;"
    //     "ldr    x1, =0x40000040;"
    //     "str    w0, [x1];"          // unmask timer interrupt
    // );
}

void disable_core_timer() { //ok
    asm volatile(
        "mov    x0, 0;"             
        "msr    cntp_ctl_el0, x0;"  // disable 
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

    for ( ; timer_id < MAX_TIMER; timer_id++){
        if (t_queue[timer_id].valid) {
            t_queue[timer_id].valid = 0;
            t_queue[timer_id].func = callback;
            t_queue[timer_id].with_arg = 0;
            t_queue[timer_id].expire_time = expire_time;

            // print_str("\nexpire_time: ");
            // print_hex(expire_time);

            // if (cur_thread->id == 2){
            //     print_str("\nTimer ID: ");
            //     print_dec(timer_id);
            // }

            enable_interrupt();

            break;
        }
    }
    
}

void add_timer_arg(void (*callback)(char*), char* arg, int expire_time){

    disable_interrupt();
    uint32_t timer_id = 0;
    
    for ( ; timer_id < MAX_TIMER; timer_id++){
        if (t_queue[timer_id].valid) {
            t_queue[timer_id].valid = 0;
            t_queue[timer_id].func_arg = callback;
            t_queue[timer_id].arg = arg;
            t_queue[timer_id].with_arg = 1;
            t_queue[timer_id].expire_time = expire_time;

            enable_interrupt();

            break;
        }
    }   
    
}

void set_timeout(char* msg, uint32_t expire_time){
    void (*callback)(char*);
    callback = async_uart_puts;

    add_timer_arg(callback, msg, expire_time);
}

void sleep(void (*wakeup_func)(), uint32_t expire_time){
    // if (cur_thread->id == 2){
    //     print_str("\nExpire: ");
    //     print_dec(expire_time);
    // }
    add_timer(wakeup_func, expire_time);
}

void print_current_time() { 
    uint32_t current_time = get_current_time();

    print_str("\nCurrent Time (after booting): ");
    print_hex(current_time);
    print_str("(secs)");
}

void one_sec_pass(){
    // async_uart_puts("\nTimer decrease");
    for (int i = 0; i < MAX_TIMER; i++){
        if (!t_queue[i].valid){
            t_queue[i].expire_time--;

            if (t_queue[i].expire_time <= 0){
                // print_current_time();
                if (t_queue[i].with_arg){
                    // print_str(t_queue[i].arg);
                    // async_uart_puts(t_queue[i].arg);
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

    // for (int i = 0; i < MAX_TIMER; i++){
    //     if (!t_queue[i].valid && cur_thread->id == 2){
    //         print_str("\nSched at: ");
    //         print_dec(t_queue[i].expire_time);
    //         break;
    //     }   
    // }

    one_sec_pass();

    asm volatile(
        "msr    cntp_tval_el0, %0;"  // set expired time
        :: "r"(get_core_freq() >> 5)
    );

    // print_str("\nhere");

    
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

uint32_t timer_empty(){
    for (int i = 0; i < MAX_TIMER; i++){
        if (!t_queue[i].valid)
            return 0;
    }
    
    return 1;
}