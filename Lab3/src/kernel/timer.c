#include "kernel/timer.h"

task_timer_t* timer_head = 0;
task_timer_t* timer_tail = 0;

int add_timer(void (*callback)(void *), void* data, int after){
    // This is for "If the timeout is earlier than the previous programed expired time, the kernel reprograms the hardware timer to the earlier one."
    int timer_set_flag = 0;
    unsigned long long cur_cnt, cnt_freq;

    // empty queue
    /*if(timer_head == timer_tail){
        timer_head = simple_malloc(sizeof(task_timer_t));
        if(timer_head == 0)
            return 0;

        timer_tail = timer_head;
    }*/
    // Invalid setting
    if(after == 0)
        return 0;
    // disable interrupt to protect critical section
    asm volatile(
        "msr daifset, 0xf;"
    );

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
    );

    // preserve data
    char *copy = simple_malloc(string_len((char*)data) + 1);
    if(copy == 0){
        asm volatile(
            "msr daifclr, 0xf;"
        );
        return 0;
    }
    string_copy(copy, (char*)data);

    task_timer_t *cur = timer_head;
    task_timer_t *temp = simple_malloc(sizeof(task_timer_t));
    // malloc fail
    if(temp == 0){
        asm volatile(
            "msr daifclr, 0xf;"
        );
        return 0;
    }
    temp->callback = callback;
    temp->data = (void*)copy;
    // set to current_time + waiting seconds, as this is be compared with current_time in the irq_handler
    temp->deadline = cur_cnt + after * cnt_freq;
    temp->next = 0;
    temp->prev = 0;


    /*uart_b2x_64(cur_cnt);
    uart_putc('\n');
    uart_b2x_64(after * cnt_freq);
    uart_putc('\n');
    uart_b2x_64(temp->deadline);
    uart_putc('\n');*/
    // this is the first timer inserted into queue
    if(timer_head == 0){
        timer_head = temp;
        timer_tail = temp;
        timer_set_flag = 1;
        // enable core0 timer interrupt
        mmio_write((long)CORE0_TIMER_IRQ_CTRL, 2);
    }

    while(!timer_set_flag){
        // insert into appropiate location based on increase-order
        if(temp->deadline <= cur->deadline){
            if(temp->deadline < cur->deadline){
                temp->prev = cur->prev;
                if(cur->prev != 0)
                    cur->prev->next = temp;
                cur->prev = temp;
                temp->next = cur;
            }
            // insert to the back of cur in order not to alter the timer
            else{
                temp->prev = cur;
                temp->next = cur->next;
                if(cur->next != 0)
                    cur->next->prev = temp;
                cur->next = temp;
            }
            // if it is inserted into the timer_head of queue
            if(cur == timer_head){
                timer_head = temp;
                timer_set_flag = 1;
            }
            break;
        }
        // traverse to last element
        if(cur == timer_tail){
            timer_tail->next = temp;
            temp->prev = timer_tail;

            timer_tail = temp;
            /*uart_puts((char*)timer_tail->data);
            uart_putc('\n');*/
            break;
        }    

        cur = cur->next;
    }

    if(timer_set_flag){
        //cnt_freq *= after;
        //cnt_freq += cur_cnt;
        // setting tval leads to cval = cur_time + tval, 
        // we set cval here in order to conform irq_handler as ot will set cval based on struct's element(which is cur_time(when added) + after)
        
        asm volatile(
            "msr cntp_cval_el0, %[var1];"
            :
            :[var1] "r" (temp->deadline)
            :
        );

        asm volatile(
            "msr cntp_ctl_el0, %[var1];"
            :
            :[var1] "r" (1)
        );
    }
    /*while(1){

    }*/
    // activate core0 timer interrupt
    //mmio_write((long)CORE0_TIMER_IRQ_CTRL, 2);

    // enable all interrupt
    asm volatile(
        "msr daifclr, 0xf;"
    );
    
    return 1;
}

void print_callback(void *str){
    unsigned long long cur_cnt, cnt_freq;
    uart_puts("The message is: ");
    uart_puts((char*)str);

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
    );

    uart_puts("   Timeout: ");
    uart_b2x_64(cur_cnt / cnt_freq);
    uart_putc('\n');
}

void settimeout(char *str, int second){
    /*char *copy = simple_malloc(string_len(str) + 1);
    string_copy(copy, str);
    uart_puts(copy);*/
    if(add_timer(print_callback, (void*)str, second) == 0)
        uart_puts("Fail to set timeout\n");
}