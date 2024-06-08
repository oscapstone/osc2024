#ifndef __TIMER_HEAP_H
#define __TIMER_HEAP_H

typedef void (*timer_callback)(void *, int);

typedef struct timer
{
    timer_callback callback;   // the function to call when the timer expires
    void *data;                // data to be passed to the callback
    int executed_time;         // the command executed time
    unsigned long long expire; // the ticket at which the timer will expire
} timer;

typedef struct timer_heap
{
    timer *arr;
    int size;
    int capacity;
} timer_heap;

timer_heap *create_timer_heap(int capacity);
void timer_heap_insertHelper(timer_heap *h, int index);
void timer_heap_heapify(timer_heap *h, int index);
timer timer_heap_extractMin(timer_heap *h);
void timer_heap_insert(timer_heap *h, timer t);

#endif