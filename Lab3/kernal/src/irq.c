#include "utils.h"
#include "printf.h"
#include "entry.h"
#include "stdio.h"
#include "str.h"
#include "irq.h"
#include "peripherals/irq.h"
#include "peripherals/mini_uart.h"

void irq_handler_timer_c();

int timer_flag=1;

const char *entry_error_messages[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1T",		

	"SYNC_INVALID_EL1h",		
	"IRQ_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		

	"SYNC_INVALID_EL0_64",		
	"IRQ_INVALID_EL0_64",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	

	"SYNC_INVALID_EL0_32",		
	"IRQ_INVALID_EL0_32",		
	"FIQ_INVALID_EL0_32",		
	"ERROR_INVALID_EL0_32"	
};

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address)
{
	//printf("%s, ESR: %x, address: %x\r\n", entry_error_messages[type], esr, address);
	puts(entry_error_messages[type]);
	puts(", ESR: 0x");
	put_long_hex(esr);
	puts(", address: 0x");
	put_long_hex(address);
	puts("\r\n");

	//read spsr_el1
	unsigned long long spsr_el1 = 0;
	asm volatile("mrs %0, spsr_el1":"=r"(spsr_el1));
	puts("spsr_el1: ");
	put_long_hex(spsr_el1);
	puts("\r\n");

	//read elr_el1
	unsigned long long elr_el1 = 0;
	asm volatile("mrs %0, elr_el1":"=r"(elr_el1));
	puts("elr_el1: ");
	put_long_hex(elr_el1);
	puts("\r\n");
	
	//esr_el1
	unsigned long long esr_el1 = 0;
	asm volatile("mrs %0, esr_el1":"=r"(esr_el1));
	puts("esr_el1: ");
	put_long_hex(esr_el1);
	puts("\r\n");

	//ec
	unsigned ec = (esr_el1 >> 26) & 0x3F; //0x3F = 0b111111(6)
	puts("ec: ");
	put_long_hex(ec);
	puts("\r\n");

	puts("enable_IRQs_1:0x");
	unsigned int* IRQs1=(unsigned int*)0x3f00b210;
	put_hex(*IRQs1);
	puts("\r\n");

	puts("DAIF:0x");
	unsigned int DAIF = 0;
	asm volatile("mrs %0, DAIF":"=r"(DAIF));
	put_hex(DAIF);
	puts("\r\n");

	// puts("HCR_EL2:0x");
	// unsigned int HCR_EL2 = 0;
	// asm volatile("mrs %0, HCR_EL2":"=r"(HCR_EL2));
	// put_hex(HCR_EL2);
	// puts("\r\n");


	if(type == 5 || type == 9){
		irq_handler_timer_c();
	}
	return ;
}

void c_recv_handler(){

    char c = (char)get32(AUX_MU_IO_REG);
    //uart_putc(c);
    c = (c=='\r'?'\n':c);

    read_buffer[read_index_tail++] = c;
    read_index_tail = read_index_tail % MAX_BUF_LEN;

    // Put into write buffer such that the received char can be echoed out
    write_buffer[write_index_tail++] = c;
    write_index_tail = write_index_tail % MAX_BUF_LEN;
	put32(AUX_MU_IER_REG,get32(AUX_MU_IER_REG) | 0x2);
}

void c_write_handler(){
	if(write_index_cur != write_index_tail){
		char c = write_buffer[write_index_cur++];
		if(c == '\n')put32(AUX_MU_IO_REG,'\r');
		put32(AUX_MU_IO_REG,(unsigned int)c);
		write_index_cur = write_index_cur % MAX_BUF_LEN;
	}
	else put32(AUX_MU_IER_REG,get32(AUX_MU_IER_REG) | ~(0x2));
}

void irq_handler_timer_c() {
	
	//puts("In timer interruption\r\n");

	unsigned long long cntpct_el0 = 0;//The register count secs with frequency
	asm volatile("mrs %0,cntpct_el0":"=r"(cntpct_el0));
	
	unsigned long long cntfrq_el0 = 0;//The base frequency
	asm volatile("mrs %0,cntfrq_el0":"=r"(cntfrq_el0));

	unsigned long long sec = cntpct_el0 / cntfrq_el0;
	if(timer_flag){
		puts("sec:");
		put_int(sec);
		puts("\r\n");
	}
	
	
	unsigned long long wait = cntfrq_el0 * 2;// wait 2 seconds
	asm volatile ("msr cntp_tval_el0, %0"::"r"(wait));//set new timer
    // fulfill the requirement set the next timeout to 2 seconds later.
	
}

void irq_general_handler_c(){
	if(get32(CORE0_INTERRUPT_SOURCE) & (1<<1)){
		irq_handler_timer_c();
		return;
	}
	else if(get32(IRQ_PENDING_1) & (1<<29)){//uart IRQ
		if(get32(AUX_MU_IIR_REG) & (1<<1)){
			c_write_handler();
		}
		else if(get32(AUX_MU_IIR_REG) & (1<<2)){
			c_recv_handler();
		}
		else{}
	}
	else{
		puts("IRQ_INVALID interrupt\r\n");
		return;
	}
}