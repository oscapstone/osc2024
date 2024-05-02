#ifndef _TIMER_H_
#define _TIMER_H_

// Define a callback function for one-shot timer.
typedef void (*one_shot_timer_callback_t)(void* arg);

typedef struct _event {
    // Enter queue time of the event.
    unsigned int enter_t;

    // The actual time it expires.
    unsigned int invoked_t;

    // Arguments passed to the callback function.
    void* arg;

    // Callback function to be invoked after timer expires.
    one_shot_timer_callback_t callback;
} Event;

// Singly linked list for the timer queue.
typedef struct _timer_queue {
    Event e;
    struct _timer_queue* next;
} TQueue;

void core_timer_enable(void);
unsigned long get_current_time(void);
void set_timer_expire(long sec);
void handle_timer_intr(void* data);
void clear_timer_intr(void);

int add_timer(one_shot_timer_callback_t callback, void* arg, unsigned int expire);
void update_timer(void);

void print_timeout_message(void* message);
void create_loop(void* data);
void exit_loop(void* data);
void delay_loop(void* data);

#endif