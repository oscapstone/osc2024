#include "timer.h"
#include "alloc.h"
#include "mini_uart.h"
#include <stddef.h>


void print_data(char* str) {
	uart_printf("%s\r\n", str);	
}

timer* head;

void push_timer(unsigned long long exp, char* str) {
	
	irq(0);

	timer* t = my_malloc(sizeof(timer));
	
	t -> exp = exp;
	for (int i = 0; ; i ++) {
		t -> data[i] = str[i];
		if (str[i] == '\0') break;
	}
	t -> callback = print_data;

	if (head == NULL) {
		head = my_malloc(sizeof(timer));
		head -> next = NULL;
	}
	
	timer* cur = head;
	while (cur -> next != NULL && cur -> next -> exp < t -> exp) {
		cur = cur -> next;
	}
	
	t -> next = cur -> next;
	cur -> next = t;
	
	if (head -> next != NULL) {
		asm volatile ("msr cntp_cval_el0, %0"::"r" (head -> next -> exp));
		asm volatile("msr cntp_ctl_el0,%0"::"r"(1));
	}
	irq(1);
}

void set_timeout(unsigned long long s, char* str) {
	
	asm volatile("msr DAIFSet, 0xf");
	unsigned long long cur_cnt, cnt_freq;
	asm volatile ( "mrs %[x], cntpct_el0" :[x] "=r" (cur_cnt));
	asm volatile ( "mrs %[x], cntfrq_el0" :[x] "=r" (cnt_freq));
	// uart_printf("freqeucy: %d\n", cnt_freq);
	cur_cnt += s * cnt_freq / 1000;
	push_timer(cur_cnt, str);
	asm volatile("msr DAIFClr, 0xf");
}
