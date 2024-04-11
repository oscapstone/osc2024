#include "timer.h"
#include "io.h"
#include "type.h"
#include "lib.h"
#include "string.h"


static struct time_t* time_head = (void*)0;

void print_time_handler(char* arg)
{

    uint64_t cycles = get_cpu_cycles();
    uint32_t freq = get_cpu_freq();
    uint64_t time = cycles / (uint64_t)freq;
    printf("\r\nTime after booting: ");
    printf_int((int)time);
    printf(" sec ");
    
}

void timer_update_handler()
{
    time_head->callback_func(time_head->arg);
    time_head = time_head->next;
    if(!time_head)
    {
        core_timer_disable();
    }
    else
    {
        uint64_t cpu_current_cycles = get_cpu_cycles();
        set_timer((time_head->timeout - cpu_current_cycles) / get_cpu_freq());
    }

}

static void print_system_timer()
{
    printf("[ ");
    printf_int(get_cpu_cycles()/get_cpu_freq());
    printf(" sec ]\t");
}

void set_timer(uint32_t sec)
{
    uint64_t cycles = get_cpu_freq() * sec;
    set_timer_asm(cycles);
}

void time_head_init()
{
    struct time_t* tmp = 0;
    time_head = tmp;
}


void add_timer(void(*callback_func)(void*), void* args, uint32_t sec)
{
    
    struct time_t* new_timer = (struct time_t*)simple_malloc(sizeof(struct time_t));

    if(new_timer == 0){
        printf("\r\n[ERROR] Out of memory");
        return;
    }
    
    new_timer->timeout = sec * get_cpu_freq() + get_cpu_cycles();
    new_timer->callback_func = callback_func;
    // new_timer->arg = args;
    strcpy(new_timer->arg, (char*)args);
    new_timer->next = (void*)0;
    // printf("\r\nnext: ");
    // printf_hex((uint64_t)new_timer->next);
    int update = 0;
    // printf("\r\ntime_head: ");
    // printf_hex((uint64_t)time_head);
    if(time_head == 0)
    {
        time_head = new_timer;
        update = 1;
    }
    else
    {
        struct time_t* curr = time_head;
        struct time_t* prev = 0;
        while(curr){
            if(curr->timeout > new_timer->timeout){
                update = 1;
                break;
            }
            prev = curr;
            curr = curr->next;
        }
        new_timer->next = curr;
        if(!prev)
        {
            time_head = new_timer;
        }
        else
        {
            prev->next = new_timer;
        }
    }
    if(update)
    {
        // printf("\r\nset");
        set_timer(sec);
    }
    printf("\r\n");
    print_system_timer();
    printf("[ADD TIMER] Set timeout at ");
    printf_int(sec);
    printf(" sec after.");
}

void print_timeout_msg(void* msg)
{
    printf("\r\n");
    print_system_timer();
    printf("[ TIMEOUT ] ");
    printf("Message: ");
    printf((char*)msg);
}
