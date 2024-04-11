#include "timer.h"
// #include "printf.h"
#include "uart.h"
#include "string.h"
#include "heap.h"
#include "tasklist.h"
#include "exception.h"

// https://hackmd.io/@hsuedw/list_head
struct list_head user_timer_list;
extern int user_timer;

void timer_router()
{
    unsigned long cntpct; // current count
    unsigned long cntfrq; // clock frequency
    asm volatile(
        "mrs %0, cntpct_el0 \n\t"
        "mrs %1, cntfrq_el0 \n\t"
        : "=r"(cntpct), "=r"(cntfrq)
        :);
    // user set timer
    if (user_timer)
    {
        timer_data *data = handle_due_timeout();
        create_task(timeout_callback, 1, data, "timer");
        // timeout_callback(data);
        // execute_tasks();
    }
    else
    {
        // timer_data *data;
        // data->execution_time = cntfrq;
        // data->system_time = cntpct;

        print_timestamp(cntpct, cntfrq);
        asm volatile(
            "mrs x0, cntfrq_el0     \n\t"
            "mov x1, 2              \n\t"
            "mul x0, x0, x1         \n\t"
            "msr cntp_tval_el0, x0  \n\t");
        // create_task(print_timestamp, 0, data, "core");
    }

    return;
}

void print_timestamp(unsigned long cntpct, unsigned long cntfrq)
// void print_timestamp(void *data)
{
    // timer_data *tdata = (timer_data *)data;
    // int timestamp = tdata->system_time / tdata->execution_time;
    int timestamp = cntpct / cntfrq;
    // printf("timestamp: %d\n", timestamp);
    uart_puts("timestamp: ");
    uart_dec(timestamp);
    uart_puts("\n");
    return;
}

void init_timer()
{
    list_init_head(&user_timer_list);
    enable_irq();
}

void set_new_timeout()
{
    // disable_irq();
    int *second;
    char *message;

    asm volatile(
        "mov %0, x10 \n\t"
        "mov %1, x11 \n\t"
        : "=r"(second), "=r"(message)
        :);

    struct user_timer *new_timer = kmalloc(sizeof(struct user_timer));
    new_timer->trigger_time = *second;
    strcpy(message, new_timer->message);

    unsigned long frequency;
    unsigned long timestamp;
    asm volatile(
        "mrs %0, cntpct_el0 \n\t"
        "mrs %1, cntfrq_el0 \n\t"
        : "=r"(timestamp), "=r"(frequency)
        :);
    unsigned long system_time = timestamp / frequency;
    new_timer->current_system_time = system_time; // 現在時間
    new_timer->execution_time = system_time;      // 設定的時間

    // if there is no previously set timer, then set it directly
    if (list_empty(&user_timer_list))
    {

        list_add_head(&new_timer->list, &user_timer_list);
        asm volatile(
            "mov x0, 1              \n"
            "msr cntp_ctl_el0, x0   \n");
        asm volatile("msr cntp_tval_el0, %0" ::"r"(frequency * new_timer->trigger_time));
        asm volatile(
            "mov x0, 2              \n"
            "ldr x1, =0x40000040    \n"
            "str x0, [x1]           \n");
    }
    else
    {

        // update the system time for each timer in the list
        for (struct user_timer *temp = (struct user_timer *)user_timer_list.next; &temp->list != &user_timer_list; temp = (struct user_timer *)temp->list.next)
        {
            temp->trigger_time -= system_time - temp->current_system_time; // 剩餘秒數
            temp->current_system_time = system_time;                       // 重設current time
        }

        struct user_timer *front = (struct user_timer *)user_timer_list.next;
        // overwrite cntp_tval_el0 if the trigger time of the new timer is less than that of the current one
        // 判斷剩餘秒數
        if (new_timer->trigger_time < front->trigger_time)
        {
            // list_crop(&front->list, &front->list);
            // kfree(front);
            list_add_head(&new_timer->list, &user_timer_list);

            asm volatile(
                "mov x0, 1              \n"
                "msr cntp_ctl_el0, x0   \n");
            asm volatile("msr cntp_tval_el0, %0" ::"r"(frequency * new_timer->trigger_time));
            asm volatile(
                "mov x0, 2              \n"
                "ldr x1, =0x40000040    \n"
                "str x0, [x1]           \n");
        }
        // find the appropriate hole to insert the new timer
        else
        {
            struct user_timer *current = front;
            struct user_timer *next = (struct user_timer *)current->list.next;

            int entered = 0;
            //timer > 1 and trigger time > current
            while ((&next->list != &user_timer_list) && (next->trigger_time < new_timer->trigger_time))
            {
                entered = 1;
                next = (struct user_timer *)current->list.next;
                current = next;
            }
            // if the above loop is entered, go one step back
            if (entered)
                current = (struct user_timer *)current->list.prev;

            __list_add(&new_timer->list, &current->list, current->list.next);
        }
    }
    // enable_irq();
    return;
}

timer_data *handle_due_timeout()
{

    unsigned long frequency;
    unsigned long timestamp;
    asm volatile(
        "mrs %0, cntpct_el0 \n\t" // current timer
        "mrs %1, cntfrq_el0 \n\t" // clock frequency
        : "=r"(timestamp), "=r"(frequency)
        :);
    unsigned long system_time = timestamp / frequency;
    struct user_timer *front = (struct user_timer *)user_timer_list.next;

    timer_data *data = kmalloc((unsigned int)sizeof(timer_data));

    data->message = front->message;
    data->system_time = system_time;
    data->execution_time = front->execution_time;

    list_crop(&front->list, &front->list); // remove this timeout
    //kfree(front);

    if (!list_empty(&user_timer_list))
    {
        // 更新剩餘秒數
        for (struct user_timer *temp = (struct user_timer *)user_timer_list.next; &temp->list != &user_timer_list; temp = (struct user_timer *)temp->list.next)
        {
            temp->trigger_time -= system_time - temp->current_system_time;
            temp->current_system_time = system_time;
        }

        front = (struct user_timer *)user_timer_list.next;
        asm volatile(
            "msr cntp_tval_el0, %0  \n\t"
            :
            : "r"(frequency * front->trigger_time));
    }
    else
    {
        user_timer = 0;
        core_timer_disable();
    }

    return data;
}

void timeout_callback(void *data)
{
    timer_data *tdata = (timer_data *)data;
    // printf("user timer due! message: %s, current time: %d, execution time: %d\n", front->message, system_time, front->execution_time);
    uart_puts("user timer due! message: ");
    uart_puts(tdata->message);
    uart_puts(", current time: ");
    uart_dec(tdata->system_time);
    uart_puts(", execution time: ");
    uart_dec(tdata->execution_time);
    uart_puts("\n");
}

/*
void core_timer_enable_c(){
    core_timer_enable();
}

void core_timer_disable_c(){
    core_timer_disable();
}*/