/**
 * Define a timer pool, because we don't have dynamic memory allocation for now.
*/

#ifndef __TIMER_H__
#define __TIMER_H__


#define NR_TIMER                    (10)
#define TIMER_DISABLE               (0)
#define TIMER_ENABLE                (1)



/**
 * We can get the absolute time by add the value of the start_time and the value of the timeout.
*/
struct timer {
    unsigned char enable;  // 0: disable, 1: enable
    unsigned long start_time;
    unsigned long timeout;
    char message[64];
};

extern struct timer timer_pool[NR_TIMER];

void timer_init();
void timer_update();
int timer_set(unsigned long timeout, char *message);




#endif // __TIMER_H__