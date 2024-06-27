#include "../include/timer.h"


task_timer_t* timer_head = 0;
task_timer_t* timer_tail = 0;

int core_timer_flag = 1;

task_timer_t *create_timer(void (*callback)(void *),void* message, int seconds){
    unsigned long long cur_cnt, cnt_freq;

    if(seconds == 0)
        return NULL;
    
    
    disable_interrupt();

    cur_cnt = read_register(cntpct_el0);
    cnt_freq = read_register(cntfrq_el0);
    
    // preserve data
    char *copy = simple_malloc(strlen((char*)message) + 1);
    if(!copy){
        
        enable_interrupt();
        uart_puts("Fail to create timer: Memory allocation error\n");
        return NULL;
    }

    strcpy(copy, (char*)message);
    task_timer_t *temp = simple_malloc(sizeof(task_timer_t));

    if(!temp){
        enable_interrupt();
        uart_puts("Fail to create timer: Memory allocation error\n");
        return NULL;
    }

    temp->callback = callback;
    temp->data = (void*)copy;
    // set to current_time + waiting seconds, as this is be compared with current_time in the irq_handler
    temp->deadline = cur_cnt + seconds * cnt_freq;
    temp->next = 0;
    temp->prev = 0;

    enable_interrupt();

    return temp;
}

int timer_add_queue(task_timer_t *temp){
    int run_timer_flag = 0;
    unsigned long long cur_cnt, cnt_freq;

    disable_interrupt();

    cur_cnt = read_register(cntpct_el0);
    cnt_freq = read_register(cntfrq_el0);

    task_timer_t *cur = timer_head;

    if(timer_head == 0){
        timer_head = temp;
        timer_tail = temp;
        // enable core0 timer interrupt
        *((volatile unsigned int*)CORE0_TIMER_IRQ_CTRL) = (0x2);
        write_register(cntp_cval_el0,temp->deadline);
        write_register(cntp_ctl_el0,1);
        enable_interrupt();
        return 1;
        
    
    }
    
    //list not empty
    while(1){
        // insert into appropiate location based on increase-order
        if(temp->deadline <= cur->deadline){
            if(temp->deadline < cur->deadline){
                temp->prev = cur->prev;
                if(cur->prev != 0)
                    cur->prev->next = temp;
                cur->prev = temp;
                temp->next = cur;
            }
            else{
                temp->prev = cur;
                temp->next = cur->next;
                if(cur->next != 0)
                    cur->next->prev = temp;
                cur->next = temp;
            }

            // if it is inserted into the timer_head of queue and have the smallest deadline
            if(cur == timer_head && temp->deadline < cur->deadline){
                timer_head = temp;
                *((volatile unsigned int*)CORE0_TIMER_IRQ_CTRL) = (0x2);
                write_register(cntp_cval_el0,temp->deadline);
                write_register(cntp_ctl_el0,1);
                enable_interrupt();
                return 1;
            }

            break;
        }
        // traverse to last element and have the biggest deadline
        if(cur == timer_tail){
            timer_tail->next = temp;
            temp->prev = timer_tail;

            timer_tail = temp;
            break;
        }    
        cur = cur->next;
    }

    enable_interrupt();
    
    return 1;
}


void timer_callback(void *str){
    unsigned long long cur_cnt, cnt_freq;
    uart_puts("The message is: ");
    uart_puts((char*)str);

    cur_cnt = read_register(cntpct_el0);
    cnt_freq = read_register(cntfrq_el0);

    uart_puts("   wake up at: ");
    uart_hex(cur_cnt / cnt_freq);
    uart_send('\n');
}

void setTimeout (char *message, int seconds) {

    //if(add_timer(timer_callback, (void*)message, seconds) == 0)
    //    uart_puts("Fail to set timeout\n");

    
    //strcpy(copy, message);
    task_timer_t *newtimer = create_timer(timer_callback, message, seconds);
    if(!newtimer){
        uart_puts("Fail to set timeout\n");
    }

    if (timer_add_queue(newtimer) == 0) {
        uart_puts("Fail to set timeout\n");
    }
    
    
}

