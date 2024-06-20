#include "timer.h"
#include "mini_uart.h"
#include "str_util.h"
#include "malloc.h"

//#define CORE0_TIMER_IRQ_CTRL 0x40000040

void core_timer_enable() {
    asm volatile(
        "mov x0, 1\n\t"
        "msr cntp_ctl_el0, x0\n\t" // enable
        "mrs x0, cntfrq_el0\n\t"
        "msr cntp_tval_el0, x0\n\t" // set expired time = 1 sec (cntp_tval = cntfreq)
        "mov x0, 2\n\t"
        "ldr x1, =0x40000040\n\t"
        "str w0, [x1]\n\t" // unmask timer interrupt
    );
    uart_send_string("core timer enabled\r\n");
}

void set_time_out_cmp(unsigned int sec) {
    /**
     * set expired time = cntp_tval = cntfreq * sec 
     * from now on, core timer will irq after sec
     */
    asm volatile("mrs x1, cntfrq_el0");
    asm volatile("mul x2, x1, %0" :: "r"(sec));
    asm volatile("msr cntp_tval_el0, x2");
}

void set_time_out_abs(unsigned int sec) {
    asm volatile("mrs x1, cntfrq_el0");
    asm volatile("mul x2, x1, %0" :: "r"(sec));
    asm volatile("msr cntp_cval_el0, x2");
}

/*void core_timer_disable() {
    asm volatile(
        "mov x0, 0\n\t"
        "msr cntp_ctl_el0, x0\n\t" // enable
        "mrs x0, cntfrq_el0\n\t"
        "msr cntp_tval_el0, x0\n\t" // set expired time
        "mov x0, 0\n\t"
        "ldr x1, =CORE0_TIMER_IRQ_CTRL\n\t"
        "str w0, [x1]\n\t" // unmask timer interrupt
    );
}*/

unsigned int get_cur_sec() {
    unsigned long cnt_timer;
    unsigned long freq_timer;
    unsigned long cur_time;

    asm volatile("mrs %0, cntpct_el0" : "=r"(cnt_timer));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq_timer));
    cur_time = cnt_timer / freq_timer;

    return cur_time;
}

void print_seconds() {
    // timer's current count (cntpct_el0)
    // the frequency of the timer (cntfrq_el0)
    unsigned int cur_time;
    char stime[16];

    cur_time = get_cur_sec();

    uart_send_string("timer interrupt: ");
    int_to_str(cur_time, stime, 10);
    uart_send_string(stime);
    uart_send_string("secs\r\n");
}

/**
 * Advanced 1: Timer Multiplexing
 * command "setTimeout MESSAGE SECONDS"
 * # An example API
     def add_timer(callback(data), after):
 */

# define TIMER_MAX_NUM 10

typedef struct timer{
    int in_use;
    unsigned int expire_sec;
    void (*callback)(char*);
    char* msg;
} timer;

static timer timer_queue[TIMER_MAX_NUM];
static unsigned int next_expire_idx = TIMER_MAX_NUM;

void timer_queue_ini() {
    for(int i = 0; i < TIMER_MAX_NUM; i++){
        timer_queue[i].in_use = 0;
        timer_queue[i].expire_sec = 10000;
    }
}

void timer_callback(char *msg){
    uart_send_string("[timer callback] ");
    uart_send_string(msg);
    uart_send_string("\r\n");
}

void add_timer(void (*callback)(char *msg), char *msg, int sec) {
    int add_pos;
    for(add_pos = 0; add_pos < TIMER_MAX_NUM; add_pos++) {
        if(timer_queue[add_pos].in_use == 0) { break; }
    }

    timer_queue[add_pos].in_use = 1;
    timer_queue[add_pos].expire_sec = sec;
    timer_queue[add_pos].callback = callback;
    timer_queue[add_pos].msg = msg;

    find_next_expire();
}

void timer_multiplex(char *cmd) {
    /**
     * cmd: setTimeout MESSAGE SECONDS
     */
    // parse command
    int p, s;
    char cmd_msg[64];
    char cmd_sec[16];
    int sec;
    
    for(p = 11; p < str_len(cmd); p++) { 
        if(*(cmd + p) == ' ') { break; }
        cmd_msg[p-11] = *(cmd + p); 
    }
    cmd_msg[p-11] = '\0';
    char *msg = my_malloc(str_len(cmd_msg) + 1);
    for(int m = 0; m < str_len(cmd_msg); m++) {
        *(msg + m) = cmd_msg[m];
    }
    p++;
    
    for(s = p ; s < str_len(cmd); s++) {
        cmd_sec[s-p] = *(cmd + s);
    }
    cmd_sec[s-p] = '\0';
    sec = str_to_int(cmd_sec);
    sec += get_cur_sec();

    add_timer(timer_callback, msg, sec);
}

void find_next_expire() {
    int found = 0;
    unsigned int min_exp_sec = 60000;

    for(int f = 0; f < TIMER_MAX_NUM; f++) {
        if(timer_queue[f].in_use == 1 && timer_queue[f].expire_sec < min_exp_sec) {
            min_exp_sec = timer_queue[f].expire_sec;
            next_expire_idx = f;
            found = 1;
        }
    }

    if(found == 0) {
        next_expire_idx = TIMER_MAX_NUM;
    }
    set_time_out_abs(min_exp_sec);
}

void timer_expire_handler() {
    int exp_pos = next_expire_idx;
    if(exp_pos < TIMER_MAX_NUM) {
        if(timer_queue[exp_pos].in_use == 1) {
            timer_queue[exp_pos].in_use = 0;
            timer_queue[exp_pos].callback(timer_queue[exp_pos].msg);
        }
    }

    find_next_expire();
}