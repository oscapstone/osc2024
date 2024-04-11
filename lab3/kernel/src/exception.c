#include "exception.h"
#include "mini_uart.h"
#include "utils.h"
#include "mmio.h"
#include "DEFINE.h"
#include "timer.h"
#include "task.h"

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

    uart_printf("Halt\n");
	while (1);
}

void el0_core_timer_entry(){
    unsigned long long cur_cnt, cnt_freq;

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
    );
    uart_printf("%d secs interrupt, freq of %d\r\n", cur_cnt, cnt_freq);
	cnt_freq *= 2;
	asm volatile(
    	"msr cntp_tval_el0, %[x];"
		:
		:[x] "r" (cnt_freq)
		:
    );
	return;
}
// This is the every 2 seconds shit, replaced for my new timer

void core_timer_entry() {

    unsigned long long cur_cnt, cnt_freq;

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
    );

	asm volatile("msr cntp_ctl_el0,%0"::"r"(0));
	asm volatile("msr DAIFSet, 0xf");

	if (head == NULL) {
		asm volatile("msr DAIFClr, 0xf");
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

	asm volatile("msr DAIFClr, 0xf");
	unsigned int* address = (unsigned int*) CORE0_TIMER_IRQ_CTRL;
	*address = 2;
}

void c_general_irq_handler(){
    unsigned int gpu_irq_src = mmio_read((long)IRQ_pending_1);
	unsigned int cpu_irq_src = mmio_read((long)CORE0_INT_SRC);

    asm volatile(
        "msr daifset, 0xf;"
    );

	// AUX int
    if(gpu_irq_src & (1 << 29)){

        unsigned int irq_status = mmio_read((long)AUX_MU_IIR_REG);
        if(irq_status & 0x4){
			change_read_irq(0);
			create_task(recv_handler, 5);
        }
        else if(irq_status & 0x2){
			change_write_irq(0);
			create_task(write_handler, 5);
		}
    }
	else if (cpu_irq_src & (0x1 << 1)) {
		unsigned int* address = (unsigned int*) CORE0_TIMER_IRQ_CTRL;
		*address = 0;

		create_task(core_timer_entry, 5);
	}
    asm volatile(
        "msr daifset, 0xf;"
    );
	execute_tasks();
    asm volatile(
        "msr daifclr, 0xf;"
    );
}

void c_el0_irq_handler(){
    unsigned int gpu_irq_src = mmio_read((long)IRQ_pending_1);
	unsigned int cpu_irq_src = mmio_read((long)CORE0_INT_SRC);

    asm volatile(
        "msr daifset, 0xf;"
    );

	if (cpu_irq_src & (0x1 << 1)) {
		el0_core_timer_entry();	
	}
    asm volatile(
        "msr daifset, 0xf;"
    );
}
