#ifndef __TIMER_HEAP_H
#define __TIMER_HEAP_H

typedef void (*timer_callback)(void *, int);

typedef struct timer{
    timer_callback callback; // the function to call when the timer expires
    void *data;              // data to be passed to the callback
    int executed_time;        // the command executed time
    int expire;              // the time at which the timer will expire
} timer;

typedef struct heap{
    timer *arr;
    int size;
    int capacity;
} heap;

heap *createHeap(int capacity);
void insertHelper(heap *h, int index);
void heapify(heap *h, int index);
timer extractMin(heap *h);
void insert(heap *h, timer t);

#endif