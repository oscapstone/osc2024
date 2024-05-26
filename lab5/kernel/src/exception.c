#include "exception.h"
#include "mini_uart.h"
#include "utils.h"
#include "mmio.h"
#include "DEFINE.h"
#include "timer.h"
#include "task.h"
#include "system_call.h"
#include "thread.h"

#include <stddef.h>

/*
extern char* write_buffer;
extern int write_ind;
extern int write_tail;

extern char* recv_buffer;
extern int recv_ind;
extern int recv_tail;
extern int flag;
*/

extern timer* head;

void exception_entry(){
	uart_printf("Entering exception handler\r\n");
    
	void *spsr1;
    void *elr1;
    void *esr1;

    asm volatile(
        "mrs %[var1], spsr_el1;"
        "mrs %[var2], elr_el1;"
        "mrs %[var3], esr_el1;"
        : [var1] "=r" (spsr1),[var2] "=r" (elr1),[var3] "=r" (esr1)
    );

    uart_printf("SPSR_EL1: %x\r\n", spsr1);
    uart_printf("ELR_EL1: %x\r\n", elr1);
    uart_printf("ESR_EL1: %x\r\n", esr1);

	return;
}

void core_timer_entry() {
    unsigned long long cur_cnt, cnt_freq;

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
    );

	asm volatile("msr cntp_ctl_el0,%0"::"r"(0));
	irq(0);
	
	if (head == NULL) {
		irq(1);
		unsigned int* address = (unsigned int*) CORE0_TIMER_IRQ_CTRL;
		*address = 2;
		return;
	}
	while (head -> next != NULL && head -> next -> exp <= cur_cnt) {
		head -> next -> callback(head -> next -> data);
		head = head -> next;
	}
	if (head -> next != NULL) {
		asm volatile("msr cntp_cval_el0, %0"::"r"(head -> next -> exp));
		asm volatile("msr cntp_ctl_el0,%0"::"r"(1));
	}

	irq(1);
	unsigned int* address = (unsigned int*) CORE0_TIMER_IRQ_CTRL;
	*address = 2;
}

void simple_core_timer_entry() {
	// irq closed before getting in
    unsigned long long cur_cnt, cnt_freq;

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
    );
	// uart_printf ("now is %d\r\n", cur_cnt / cnt_freq);
	asm volatile("msr cntp_tval_el0, %0"::"r"(cnt_freq));
	// unsigned int* address = (unsigned int*) CORE0_TIMER_IRQ_CTRL;
	// *address = 2;

	schedule();
	// uart_printf ("I'm back\r\n");
}


void c_general_irq_handler(){
    unsigned int gpu_irq_src = mmio_read((long)IRQ_pending_1);
	unsigned int cpu_irq_src = mmio_read((long)CORE0_INT_SRC);

	irq(0);

    if(gpu_irq_src & (1 << 29)){

        unsigned int irq_status = mmio_read((long)AUX_MU_IIR_REG);
        if(irq_status & 0x4){
			// uart_printf ("[DEBUG] irq read\r\n");
			// change_read_irq(0);
			// create_task(recv_handler, 5);
			recv_handler();
        }
        else if(irq_status & 0x2){
			// uart_printf ("[DEBUG] irq write\r\n");
			// change_write_irq(0);
			// create_task(write_handler, 5);
			write_handler();
		}
    }
	else if (cpu_irq_src & (0x1 << 1)) {
		// unsigned int* address = (unsigned int*) CORE0_TIMER_IRQ_CTRL;
		// *address = 0;
		// create_task(simple_core_timer_entry, 5);
		simple_core_timer_entry();
	}
	irq(1);
	// execute_tasks();
}

void c_undefined_exception() {
	uart_printf("This is a undefined exception. Proc hang\r\n");
	while (1);
}

void c_system_call_handler(trapframe_t* tf) {
	irq(0);
	int id = tf -> x[8];
	// uart_printf ("having exception of id %d\r\n", id);
	if (id == 0) {
		tf -> x[0] = do_getpid();
	}
	else if (id == 1) {
		irq(1);
		tf -> x[0] = do_uart_read(tf -> x[0], tf -> x[1]);
	}
	else if (id == 2) {
		irq(1);
		tf -> x[0] = do_uart_write(tf -> x[0], tf -> x[1]);
	}
	else if (id == 3) {
		tf -> x[0] = do_exec(tf -> x[0], tf -> x[1]);
	}
	else if (id == 4) {
		tf -> x[0] = do_fork(tf);
	}
	else if (id == 5) {
		do_exit();
	}
	else if (id == 6) {
		uart_printf ("mailboxing\r\n");
		tf -> x[0] = do_mbox_call(tf -> x[0], tf -> x[1]);
	}
	else if (id == 7) {
		do_kill(tf -> x[0]);	
	}
	else {
		uart_printf ("This is unexpected systemcall, proc hang\r\n");
		while (1);
	}
	// uart_printf ("Exception ended with return value = %d\r\n", tf -> x[0]);
	irq(1);
}
