#include "list.h"

struct user_timer
{
    struct list_head list;

    unsigned long execution_time;
    unsigned long current_system_time;
    unsigned int trigger_time;

    char message[50];
};
typedef struct timer_data
{
    char *message;
    unsigned long system_time;
    unsigned long execution_time;
} timer_data;

void timer_router();

void print_timestamp(unsigned long cntpct, unsigned long cntfrq);
//void print_timestamp(void *data);

void init_timer();
void set_new_timeout();
timer_data *handle_due_timeout();
void timeout_callback(void *data);
extern core_timer_enable();
extern core_timer_disable();
