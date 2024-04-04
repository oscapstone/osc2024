#include "interrupt.h"
#include "stddef.h"
#include "timer.h"
#include "uart.h"
#include "stdint.h"

#define INSERT_TAIL

struct tasklet_struct tl_pool[2] = {
    {NULL, UART_TASKLET, uart_tasklet, 0},
    {NULL, TIMER_TASKLET, timer_tasklet, 0}};
struct tasklet_head tl_head = {0};
volatile unsigned long cur_tl_priority = 0;

/* Initialize the tasklet. */
void tasklet_init(void)
{
    cur_tl_priority = NO_TASKLET;

    /* Initialize the tasklet list. */
    tl_head.head = NULL;
    tl_head.tail = &tl_head.head;
}

/* Add a tasklet to the tasklet list. */
void tasklet_add(struct tasklet_struct *tl)
{
#ifdef INSERT_TAIL
    tl->next = NULL;
    *(tl_head.tail) = tl;
    tl_head.tail = &tl->next;
#else // LIFO
    tl->next = tl_head.head;
    tl_head.head = tl;
#endif
}

/* Select the tasklet with highest priority and do it. */
void do_tasklet(void)
{
    unsigned long prev_priority = cur_tl_priority;
    struct tasklet_struct **tl; // the tasklet to be done.
    struct tasklet_struct **tl_iter; // the iterator of the tasklet list.
    struct tasklet_struct *cur_tl = NULL;
    bool todo;

    /* Select the highest priority taskelt in the tl_head until there's no tasklet which priority greater than prev_priority */
    while (1)
    {
        tl = &tl_head.head;
        tl_iter = &tl_head.head;
        todo = false;
        cur_tl = NULL;

        while (*tl_iter)
        {
            if ((*tl_iter)->state > prev_priority && (*tl_iter)->state > cur_tl_priority)
            {
                cur_tl_priority = (*tl_iter)->state;
                tl = tl_iter;
                todo = true;
            }
            tl_iter = &(*tl_iter)->next;
        }

        if (!todo)
            break;
        
        /* Remove the tasklet from the tl_head list */
#ifdef INSERT_TAIL
        if (!(*tl)->next) // if `*tl` is the last element, we should move the tl_head.tail
            tl_head.tail = tl;
#endif
        cur_tl = *tl;
        *tl = cur_tl->next;
        /* Do the tl tasklet. */
        cur_tl->func(cur_tl->data);
    }

    /* Restore the previous tasklet priority */
    cur_tl_priority = prev_priority;
}
