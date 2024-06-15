#include "rpi_base.h"
#include "utility.h"
#include "stdint.h"


#ifndef _TIMER_H
#define _TIMER_H


#define UART_IRQ_PRIORITY  1
#define TIMER_IRQ_PRIORITY 0
#define INTERRUPT_SOURCE_CNTPNSIRQ (1<<1)
#define INTERRUPT_SOURCE_GPU (1<<8)
#define IRQ_PENDING_1_AUX_INT (1<<29)

// basic 2
#define CORE0_TIMER_IRQ_CTRL            ((unsigned int*)0x40000040) // Core0 Timer Interrupt Controller 
#define CORE0_TIMER_IRQ_CTRL1            (0x40000040) // for string input
// #define CORE1_TIMER_IRQ_CTRL            ((unsigned int*)0x40000044)
// #define CORE2_TIMER_IRQ_CTRL            ((unsigned int*)0x40000048)
// #define CORE3_TIMER_IRQ_CTRL            ((unsigned int*)0x4000004C)

#define CORE0_INTR_SRC                  ((unsigned int*)0x40000060) //Core0 Interrupt Source
// #define CORE1_INTR_SRC                  ((unsigned int*)0x40000064)
// #define CORE2_INTR_SRC                  ((unsigned int*)0x40000068)
// #define CORE3_INTR_SRC                  ((unsigned int*)0x4000006C)

typedef struct timer_event {
    struct list_head listhead;
    unsigned long long interrupt_time;  //會設置為timer準備interrupt的時間
    void *callback; // 流程: interrupt -> timer_callback -> callback(args) 用來呼叫到callback function
    char* args; // 放置時間到輸出的文字
} timer_event_t;


void timer_list_init();
void timer_init(void);
void core_timer_hadler();
void el1_interrupt_enable();// umask all DAIF
void el1_interrupt_disable();
void The2sTimer(char* str);

void core_timer_intr_handler();
//Advanced 1 - Timer Multiplexing
//我們需要有一個queue去存計時器的個數
//定時器事件結構

void add_timer_by_tick(uint64_t tick, void *callback, void *args_struct);
void add_timer(void *callback, unsigned long long timeout, char* args);
void timer_event_callback(timer_event_t * timer_event);
unsigned long long get_tick_plus_s(unsigned long long second);
void set_core_timer_interrupt(unsigned long long expired_time);
void set_core_timer_interrupt_by_tick(unsigned long long tick);
void timer_set2sAlert(char* str);
void timer_list_init();
int  timer_list_get_size();
void delay();




#endif