#define BUFFER_SIZE 1024
#define MAX_TIMER 10
typedef void (*timer_callback_t)(char* data);

struct timer{
    timer_callback_t callback;
    char data[BUFFER_SIZE];
    unsigned long expires;
};

void add_timer(timer_callback_t callback, char* data, unsigned long after);
void setTimeout_callback(char* data);
void setTimeout_cmd();
unsigned long get_current_time();
void set_timer_interrupt(unsigned long second);
void disable_core_timer();
void timer_handler();