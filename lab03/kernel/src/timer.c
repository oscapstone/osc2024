#include "timer.h"
#include "io.h"
#include "type.h"

void print_time_handler()
{
    static int i = 0;
    uint64_t cycles = get_cpu_cycles();
    uint32_t freq = get_cpu_freq();
    uint64_t time = cycles / (uint64_t)freq;
    printf("\r\nTime after booting: ");
    printf_int((int)time);
    printf(" sec ");
    if(i < 2){
        i++;
        set_timer(2);
    }
    else{
        core_timer_disable();
    }
    
}

void set_timer(uint32_t sec)
{
    uint64_t cycles = get_cpu_freq() * sec;
    set_timer_asm(cycles);
}
