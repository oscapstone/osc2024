#include "timer.h"
#include "alloc.h"
#include "mini_uart.h"
#include "task.h"
#include "irq.h"

#define DEFAULT_TIMER 0xFFF

// #define CORE0_TIMER_IRQ_CTRL 0x40000040
TQueue* q_head;

void core_timer_enable(void) {
    // Initialize time queue.
    q_head = NULL;

    // Set interrupt interval to a very large time initially.
    int interval = 0xFFFFFFFF;

    asm volatile(
        "mov x0, 1;"
        // Control register for the physical timer on the current core.
        "msr cntp_ctl_el0, x0;" // enable

        // cntfrq_el0 holds the frequency in Hz at which the system
        // counter increments.
        "mrs x0, cntfrq_el0;"
        //"mul x0, x0, %0;"
        //"msr cntp_tval_el0, x0;" // set expired time
        "mov x0, 2;"
        "ldr x1, =0x40000040;"

        // Stores the value from "x0"(which is the "w0" register, the 32-bit view of "x0").
        // Writing 2 into the register unmasks the timer interrupt.
        "str x0, [x1];" // unmask timer interrupt

        :
        : "r" (interval)
        :

    );
}

unsigned long get_current_time(void) {
    long long current_time;
    long long freq;

    asm volatile(
        "mrs x0, cntpct_el0;"
        "mov %0, x0;"
        "mrs x1, cntfrq_el0;"
        "mov %1, x1;"

        // Output operands.
        : "=r" (current_time), "=r" (freq)
        // No input operands.
        :
        // Clobbered registers.
        : "x0", "x1"
    );

    return (unsigned long)(current_time / freq);
}

void set_timer_expire(long sec) {

    asm volatile(
        "mrs x0, cntfrq_el0;"
        "mul x0, x0, %0;"
        "msr cntp_tval_el0, x0;"

        // Output operand.
        :
        // Input operand.
        : "r" (sec)
        // Clobbered registers.
        : "x0"
    );
}

int p = 1;

void handle_timer_intr(void* data) {
    disable_el1_interrupt();
    uart_send_string("Timer invoked!\r\n");

    TQueue* cur = q_head;
    if (cur == NULL) {
        uart_send_string("Timer queue empty!\r\n");
    // Check which event should be invoked.
    } else {
        unsigned int cur_time = get_current_time();

        // Check which event expired.
        while (cur != NULL) {
            if (cur->e.invoked_t <= cur_time) {
                task* t = (task *)simple_malloc(sizeof(task));
                uart_send_string("Command executed time: ");
                uart_send_uint(cur->e.enter_t);
                uart_send_string("\r\n");
                uart_send_string("Event message: ");

                t->intr_func = cur->e.callback;
                t->arg = cur->e.arg;
                t->priority = p++;
                enqueue_task(t);
                uart_send_string("Current time: ");
                uart_send_uint(cur_time);
                uart_send_string("\r\n");

                cur = cur->next;
            } else {
                break;
            }
        }       
        

        q_head = cur;
    }

    update_timer();
    execute_task();
}

// Register the callback function into the timer queue.
int add_timer(one_shot_timer_callback_t callback, void* arg, unsigned int expire) {
    // Allocate memory for the argument in case it was erased before executing the interrupt.
    char* copy_arg = simple_malloc(sizeof(char) * strlen((char *)arg));
    strcpy(copy_arg, (char *)arg);

    TQueue* new = (TQueue *)simple_malloc(sizeof(TQueue));
    new->next = NULL;
    new->e.enter_t = get_current_time();
    new->e.invoked_t = new->e.enter_t + expire;
    new->e.callback = callback;
    new->e.arg = copy_arg;

    // Queue is empty.
    if (q_head == NULL) {
        q_head = new;
        set_timer_expire(expire);
    } else {
        TQueue* cur = q_head;

        // Sorted queue from smallest invoked time to largest
        // to avoid inaccuracies when multiple timers are active.
        if (cur->e.invoked_t > new->e.invoked_t) {
            new->next = cur;
            q_head = new;
        } else {
            while (cur->next != NULL) {
                if (cur->next->e.invoked_t <= new->e.invoked_t) {
                    cur = cur->next;
                } else {
                    break;
                }
            }
            new->next = cur->next;
            cur->next = new;
        }

        update_timer();
    }
}

// Check the invoked time of first element within the queue.
// If queue is empty, reset the timer to 20 seconds.
void update_timer(void) {
    unsigned int cur_time = get_current_time();

    if (q_head == NULL) {
        set_timer_expire(DEFAULT_TIMER);
    } else {
        // Expire immediately if the event is overdue.
        if (q_head->e.invoked_t <= cur_time) {
            set_timer_expire(0);
        } else {
            set_timer_expire(q_head->e.invoked_t - cur_time);
        }
    }
}

void clear_timer_intr(void) {
    unsigned long sec = 0xFFF;

    asm volatile (
        "mrs x0, cntfrq_el0;"
        "mul x0, x0, %0;"
        "msr cntp_tval_el0, x0;"

        // Output operand.
        :
        // Input operand.
        : "r" (sec)
        // Clobbered registers.
        : "x0"
    );
}

void print_timeout_message(void* message) {
    uart_send_string((char *)message);
    uart_send_string("\r\n");
}


static volatile int flag = 1;
void create_loop(void* data) {
    uart_send_string("In create loop\r\n");
    while (flag);
    uart_send_string("Exit create loop \r\n");
}

void exit_loop(void* data) {
    uart_send_string("In exit loop\r\n");
    flag = 0;
}

void delay_loop(void* data) {
    // delay(10000);
    uart_send_string("In delay loop\r\n");
    while (1);
}