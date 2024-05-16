#include "interrupt.h"
#include "mini_uart.h"
#include "timer.h"
#include "heap.h"
#include "memory.h"
#include "thread.h"
#include "signal.h"
#include <stddef.h>

/* task queue */

static int irq_running = 0; // used to record the current irq priority

irq_task_t* irq_task_head = NULL;

int add_irq_task(int (*callback)(void), uint64_t priority){
  // disable_irq();
  irq_task_t* new_task = (irq_task_t*)kmalloc(sizeof(irq_task_t));
  if(!new_task){
    uart_sends("Fail malloc");
    return -1;
  }

  new_task->priority = priority;
  new_task->callback = callback;
  new_task->next = NULL;

  irq_task_t* current = irq_task_head;
  irq_task_t* prev = NULL;

  while(current!= NULL && new_task->priority <= current->priority){
    prev = current;
    current = current->next;
  }

  if(!prev){
    new_task->next = irq_task_head;
    irq_task_head = new_task;
  }
  else{
    prev->next = new_task;
    new_task->next = current;
  }
  // enable_irq();
  return 0;
}

/*
  Used to test preemption for irq task
  High -> Low: change the priority of timer to 10
  Low -> Hign: change the priority of timer to 50
*/
int flag = 0;
void irq_test(){
  if(flag < 3){
    set_timeout("test", -1);
    flag++;
  }
  return;
}

/*
  if someone works
    if the current task's priority is the largest:
      directly do it and return;
    else return;
  else:
    do all tasks, even someone add task, if the priority is lower, the task would add to the list;
*/
int exec_task(int priority){
  int tmp = irq_running;
  if(irq_running){
    if(priority > irq_running){
      disable_irq();
      irq_task_t *task = irq_task_head;
      irq_running = task->priority;
      irq_task_head = irq_task_head->next;
      enable_irq();
      task->callback();
      kfree(task);
      irq_running = tmp;
    }
    return 0;
  }
  else{
    while(irq_task_head){
      disable_irq();
      irq_task_t *task = irq_task_head;
      irq_running = task->priority;
      irq_task_head = irq_task_head->next;
      enable_irq();
      // irq_test();
      task->callback();
      // free
      kfree(task);
    }
    irq_running = 0;
  }
  return 0;
}

int irq_entry(){
  disable_irq();
  int priority = 0;
  if(*IRQ_PEND1_REG & (1 << 29)){ // AUX INT
    priority = mini_uart_irq_handler();
  }
  else if(*CORE0_IRQ_SOURCE & 0x2){ // CNTPSIRQ INT
    if(get_current()->next != get_current())
      schedule();

    core_timer_disable();
    priority = 10;
    add_irq_task(timer_irq_handler, priority);
    core_timer_enable();
  }
  enable_irq();
  exec_task(priority);

  signal_check();
  return 0;
}

int lower_irq_entry(){
  disable_irq();
  int priority = 0;
  if(*IRQ_PEND1_REG & (1 << 29)){ // AUX INT
    priority = mini_uart_irq_handler();
  }
  else if(*CORE0_IRQ_SOURCE & 0x2){ // CNTPSIRQ INT
    schedule();
    core_timer_disable();
    priority = 15;
    add_irq_task(lower_timer_handler, priority);
    core_timer_enable();

  }
  enable_irq();
  exec_task(priority);

  signal_check();
  return 0;
}

void enable_irq(){
  asm volatile("msr    daifclr, #0xf;");
  mini_uart_irq_enable();
}

void disable_irq(){
  asm volatile("msr    daifset, #0xf;");
}

/*
  core timer interrupt
*/
void core_timer_enable(){
  uint64_t tmp;
  asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
  tmp |= 1;
  asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
  asm volatile(
    "mov x0, 1;"
    "msr cntp_ctl_el0, x0;" // enable
    "mrs x0, cntfrq_el0;"
    "msr cntp_tval_el0, x0;" // set expired time
    "mov x0, 2;"
    "ldr x1, =0x40000040;"
    "str w0, [x1];" // unmask timer interrupt
  );
}

void core_timer_disable(){
  asm volatile("msr cntp_ctl_el0, %0" :: "r"(0));
}

int timer_irq_handler(){
  uint64_t current_time = get_time();

  while(timer_head != NULL && timer_head->time <= current_time){
    timer_t* tmp = timer_head;
    timer_head = tmp->next;
    tmp->callback((void*)tmp->data);
    kfree(tmp);
  }

  asm volatile("msr cntp_tval_el0, %0;" :: "r"(get_freq() >> 5));
  return 0;
}

int lower_timer_handler(){
  uint64_t current_time = get_time();

  while(timer_head != NULL && timer_head->time <= current_time){
    timer_t* tmp = timer_head;
    timer_head = tmp->next;
    tmp->callback((void*)tmp->data);
    kfree(tmp);
  }

  asm volatile("msr cntp_tval_el0, %0;" :: "r"(get_freq() >> 5));
  return 0;
}

/*
  mini uart interrupt: receive and transmit.
  Return Value: priority of each interrupt.
*/
int mini_uart_irq_handler(){
  // uart_sends("mini_uart irq handler!!!\n");
  int p = 0;
  if(*AUX_MU_IIR_REG & 0x2){
    // Transmitter
    *AUX_MU_IER_REG &= ~(0x2);
    // uart_sends("In tx interrupt!!!\n");
    p = 20;
    add_irq_task(uart_tx_handler, p);
  }
  else if(*AUX_MU_IIR_REG & 0x4){
    // Receiver
    *AUX_MU_IER_REG &= ~(0x1);
    // uart_sends("In rx interrupt!!!\n");
    p = 30;
    add_irq_task(uart_rx_handler, p);
  }
  return p;
}

void mini_uart_irq_enable(){
  // *AUX_MU_IER_REG |= 1;
  *IRQs1 |= (1 << 29);
}

void mini_uart_irq_disable(){
  *AUX_MU_IER_REG &= ~(0x3);
  *DISABLE_IRQs1 |= (1 << 29);
}

int uart_rx_handler(){
  *AUX_MU_IER_REG &= ~(0x1);
  char c = (char)*AUX_MU_IO_REG;
  uart_rx_buffer[uart_rx_max] = (c == '\r' ? '\n': c);
  uart_rx_max = (uart_rx_max + 1)%BUFFER_SIZE;
  *AUX_MU_IER_REG |= (0x1);
  return 0;
}

int uart_tx_handler(){
  // uart_sends("=====start=====\n");
  // unsigned int cycles = 1000000000; // test premmption for qemu
  // unsigned int cycles = 10000000; // test premmption for raspi3b+
  // while (cycles--)
  // {
  //   asm volatile("nop");
  // }
  if(uart_tx_index != uart_tx_max){
    *AUX_MU_IO_REG = uart_tx_buffer[uart_tx_index];
    uart_tx_index = (uart_tx_index + 1) % BUFFER_SIZE;
    *AUX_MU_IER_REG |= (0x2);
  }
  else{
    *AUX_MU_IER_REG &= ~(0x2);
  }
  // uart_sends("======end======\n");
  return 0;
}
