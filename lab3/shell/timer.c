#include "header/timer.h"
#include "header/allocator.h"
#include "header/uart.h"
#include "header/irq.h"
#include "header/utils.h"

timer_t *timer_head = NULL;

void add_timer(timer_t *new_timer) {
    
    // 每次current都從頭找
    timer_t *current = timer_head;

    // DAIFSet : Disable interrupts 保護 critical section
    asm volatile("msr DAIFSet, 0xf");
    
    // 如果list是空或是新的timer是最快的
    if (!timer_head || new_timer->expiry < timer_head->expiry) {
        new_timer->next = timer_head;
        new_timer->prev = NULL;
        if (timer_head) {
            timer_head->prev = new_timer;
        }
        timer_head = new_timer;

        // cntp_cval_el0: A compared timer count
        // If cntpct_el0 >= cntp_cval_el0, interrupt the CPU core.
        asm volatile ("msr cntp_cval_el0, %0"::"r"(new_timer->expiry));
        // enable timer
        asm volatile("msr cntp_ctl_el0,%0"::"r"(1));

        // Enable interrupts
        asm volatile("msr DAIFClr, 0xf");
        return;
    }
    
    // 判斷哪個timer先到期
    // ex : 1 -> 2, 3
    while (current->next && current->next->expiry < new_timer->expiry) {
        current = current->next;
    }
    
    // 1->2->3
    // Insert the new timer
    new_timer->next = current->next;
    new_timer->prev = current;
    
    // 如果後面還有
    // ex : 1 -> 4, 2
    if (current->next) {
        current->next->prev = new_timer;
    }
    current->next = new_timer;
    
    // DAIFClr : Enable interrupts
    asm volatile("msr DAIFClr, 0xf");
}


void create_timer(timer_callback callback, void* data, uint64_t after) {
	//Allocate memory for the timer
	timer_t* timer = simple_malloc(sizeof(timer_t));
	if(!timer) {
		return;
	}

	// callback -> print_message func.
	timer->callback = callback;
	// data -> after(幾秒)
	timer->data = data;

	// 計算expiry time
	uint64_t current_time, cntfrq;
	asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
	asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq));
	timer->expiry = current_time + after * cntfrq;
	
	
	// 把timer加入至list
	add_timer(timer);
}


void print_message(void *data) {
	char *message = data;
	uint64_t current_time, cntfrq;
	// cntpct_el0 : a process sleeps for ...seconds.
	// cntfrq_el0 : the periodic timer’s frequency is ...Hz.
	asm volatile("mrs %0, cntpct_el0" : "=r"(current_time));
	asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));
	uint64_t seconds = current_time / cntfrq;

	uart_send_string("Timeout message: ");
	uart_send_string(message);
	uart_send_string(" occurs at ");
	uart_hex(seconds);
	uart_send_string("\n");
}

// adv1
// 過了幾個seconds執行print_message()
void setTimeout(char *message,uint64_t seconds) {
	
	// allo meomry
	char *message_copy = utils_strdup(message);

	if(!message_copy){
		return;
	}

	if (!timer_head) {
		// core_timer_interrupt : enable core timer's interrupt
		// 4個core, 所以2^2 = 4
		unsigned int value = 2;
		unsigned int* address = (unsigned int*) CORE0_TIMER_IRQ_CTRL;
		*address = value;
	}
	
	create_timer(print_message, message_copy, seconds);
}

// Basic2
void timer_set2sAlert(char* str)
{
    unsigned long long cntpct_el0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t": "=r"(cntpct_el0)); // tick auchor
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); // tick frequency
    uart_sendline("[Interrupt][el1_irq][%s] %d seconds after booting\n", str, cntpct_el0/cntfrq_el0);
    add_timer(timer_set2sAlert,2,"2sAlert");
}

