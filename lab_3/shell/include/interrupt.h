#define BASE_INTERRUPT_REGISTER 0x7E00B000
#ifndef NULL
    #define NULL 0
#endif
#define CORE0_TIMER_IRQ_CTRL 0x40000040

#define IRQ_BASIC_PENDING         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000200))
#define IRQ_PENDING_1        ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000204))
#define IRQ_PENDING_2         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000208))
#define FIQ_CONTROL         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x0000020C))
#define ENABLE_IRQS_1         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000210))
#define ENABLE_IRQS_2         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000214))
#define ENABLE_BASIC_IRQS        ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000218))
#define DISABLE_IRQS_1         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x0000021C))
#define DISABLE_IRQS_2        ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000220))
#define DISABLE_BASIC_IRQS        ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000224))

typedef struct Timer{
    unsigned long long trigger_tick;
    void* callback_function;
    void* argument;
    struct Timer* next;
}Timer;

int print_boot_timer();
void interrupt_entry();
unsigned long long get_current_tick();
void disable_timer_interrupt();
void enable_timer_interrupt();
void print_time_out(int delay);
void set_time_out(char* message, unsigned long long seconds);
void core_timer_init();
void _set_time_out(void (*callback_func)(void *), char* message, unsigned long long delay);
void print_time();