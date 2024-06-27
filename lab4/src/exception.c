#include "../include/mini_uart.h"
#include "../include/timer.h"
#include "../include/timer_utils.h"
#include "../include/exception.h"
#include "../include/peripherals/mini_uart.h"
#include "../include/task_queue.h"
#include "../include/list.h"
#include "../include/mem_utils.h"
#include <stddef.h>

void enable_interrupt() { asm volatile("msr DAIFClr, 0xf"); }
void disable_interrupt() { asm volatile("msr DAIFSet, 0xf"); }

static unsigned int nested_interrupted_task = 0; 

static void show_exception_info()
{
	uint64_t spsr = read_sysreg(spsr_el1);
	uint64_t elr  = read_sysreg(elr_el1);
	uint64_t esr  = read_sysreg(esr_el1);
	uart_send_string("spsr_el1: 0x");
	uart_hex(spsr);
	uart_send_string("\r\n");
	uart_send_string("elr_el1: 0x");
	uart_hex(elr);
	uart_send_string("\r\n");
	uart_send_string("esr_el1: 0x");
	uart_hex(esr);
	uart_send_string("\r\n");
}

void exception_invalid_handler()
{
	disable_interrupt();
	uart_send_string("---In exception_invalid_handler---\r\n");
	show_exception_info();
	enable_interrupt();
}

void except_el0_sync_handler() 
{
	disable_interrupt();
	uart_send_string("---In exception_el0_sync_handler---\r\n");
	show_exception_info();
	enable_interrupt();
}

void except_el1_sync_handler() 
{
	disable_interrupt();
	uart_send_string("---In exception_el1_sync_handler---\r\n");
	show_exception_info();
	enable_interrupt();
}

/* for Advanced 2: nested interrupt and preemption */

typedef void (*task_callback)(void);

static LIST_HEAD(task_queue);

struct task_event {
	struct list_head head;
	int priority;
	task_callback cb;
};

static struct task_event *task_event_create(task_callback cb, int priority)
{
	struct task_event *event = chunk_alloc(sizeof(struct task_event));
	event->cb = cb;
	event->priority = priority;
	return event;
}

static void task_event_add_queue(struct task_event *event)
{
	struct list_head *curr = task_queue.next;

	while (curr != &task_queue && (((struct task_event *)curr)->priority <= event->priority))
		curr = curr->next;
	list_add(&event->head, curr->prev, curr);
}

// static void print_task()
// {
// 	struct list_head *curr = task_queue.next;
// 	while (curr != &task_queue) {
// 		struct task_event *tmp_event = (struct task_event *)curr;

// 	}
// }

static void exec_task()
{
	if (!list_is_empty((struct list_head *)&task_queue)) {
		struct task_event *event = (struct task_event *)task_queue.next;
		// nested_interrupted_task++;
		// uart_send_string("# of nested interrupted event: ");
		// uart_hex(nested_interrupted_task);
		// uart_send_string("\r\n");
		enable_interrupt();
		event->cb();
		disable_interrupt();
		// uart_send_string("# of nested interrupted event: ");
		// uart_hex(nested_interrupted_task);
		// uart_send_string("\r\n");
		// nested_interrupted_task--;

		list_del((struct list_head *)event);
		chunk_free((char *)event);
	}
}

void timer_handler()
{
	// disable_interrupt();
	// uart_send_string("In timer handle\n");
	// uint64_t current_time = get_current_time();
	// uart_send_string("Current time: ");
	// uart_hex(current_time);
	// uart_send_string(" s  \r\n");
	// set_expired_time(2);
	// enable_interrupt();
}

void exception_el1_irq_handler()
{
	disable_interrupt();
	nested_interrupted_task++;
	// uart_send_string("Before handler: # of nested interrupted event: ");
	// // uart_hex(nested_interrupted_task);
	// uart_send_string("\r\n");
	struct task_event *event = NULL;
	if (*CORE0_INTERRUPT_SOURCE & 0x2) {
		disable_timer_interrupt();
		// timer_handler();
		// timer_interrupt_handler();
		// uart_send_string("> ");
		event = task_event_create(timer_interrupt_handler, 1);
	}
	else if (*IRQ_PENDING_1 & (1 << 29)) {
		if (*AUX_MU_IIR_REG & 0x4) {
			clr_rx_interrupts();
			// uart_rx_handler();
			event = task_event_create(uart_rx_handler, 3);
		}
		else if (*AUX_MU_IIR_REG & 0x2) {
			clr_tx_interrupts();
			// uart_tx_handler();
			event = task_event_create(uart_tx_handler, 3);
		}
	}

	task_event_add_queue(event);
	exec_task();
	// uart_send_string("After handler: # of nested interrupted event: ");
	// uart_hex(nested_interrupted_task);
	// uart_send_string("\r\n");
	nested_interrupted_task--;
	enable_interrupt();
}