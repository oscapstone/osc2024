#include "utils.h"
#include "printf.h"
#include "entry.h"
#include "stdio.h"
#include "str.h"
#include "irq.h"
#include "peripherals/irq.h"
#include "peripherals/mini_uart.h"
#include "sysc.h"
#include "scheduler.h"
#include "stddef.h"

int timer_flag=0;

void irq_handler_timer_c();

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


	if(type == 4){
		reset(0x400);
		delay(0x400);
	}
	return ;
}

void c_recv_handler(){
    while(!(get32(AUX_MU_LSR_REG) & 0x01) ){
        // if bit 0 is set, break and return IO_REG
        asm volatile("nop");
    }

    char c = (char)get32(AUX_MU_IO_REG);
    //uart_putc(c);
    c = (c=='\r'?'\n':c);

    read_buffer[read_index_tail++] = c;
    read_index_tail = read_index_tail % MAX_BUF_LEN;

    // Put into write buffer such that the received char can be echoed out
    write_buffer[write_index_tail++] = c;
    write_index_tail = write_index_tail % MAX_BUF_LEN;

    //task_create_DF0(c_write_handler, 2);
    put32((long)AUX_MU_IER_REG, get32(AUX_MU_IER_REG) | 0x2);
}

void c_write_handler(){
    //mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | 0x2);

    while(write_index_cur != write_index_tail){
        char c = write_buffer[write_index_cur++];

        // If no CR, first line of output will be moved right for n chars(n=shell command just input), not sure why
        if(c == '\n'){
            while(!(get32(AUX_MU_LSR_REG) & 0x20) ){
                asm volatile("nop");
            }
			put32(AUX_MU_IO_REG,'\r');
        }

        // check if FIFO can accept at least one byte after sending one character
        while(!(get32(AUX_MU_LSR_REG) & 0x20) ){
            // if bit 5 is set, break and return IO_REG
            asm volatile("nop");
        }
        //*AUX_MU_IO_REG = c;
		put32(AUX_MU_IO_REG,(unsigned int)c);
        
        write_index_cur = write_index_cur % MAX_BUF_LEN;
    }
    // enable receiver interrupt
    put32((long)AUX_MU_IER_REG, get32(AUX_MU_IER_REG) | (0x1));
}

void irq_handler_timer_c() {

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
	kill_zombies();
	unsigned long long tmp;
	asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
	tmp |= 1;
	asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
	if(running_ring != NULL && running_ring->next != running_ring)schedule();
	unsigned long long wait = (cntfrq_el0 >> 5);// wait 2 seconds
	asm volatile ("msr cntp_tval_el0, %0"::"r"(wait));//set new timer
    // fulfill the requirement set the next timeout to 2 seconds later.
	return;
}

void irq_general_handler_c(){
	if(get32(CORE0_INTERRUPT_SOURCE) & (1<<1)){
		irq_handler_timer_c();
		return;
	}
	else if(get32(IRQ_PENDING_1) & (1<<29)){//uart IRQ

		if(get32(AUX_MU_IIR_REG) & (1<<2)){ //reseive

			//puts("in receive\r\n");
			c_recv_handler();
			put32(AUX_MU_IER_REG,get32(AUX_MU_IER_REG) & ~(0x1));
			//mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG & ~(0x1));
			
		}
		else if(get32(AUX_MU_IIR_REG) & (1<<1)){

			
			c_write_handler();
			put32(AUX_MU_IER_REG,get32(AUX_MU_IER_REG) & ~(0x2));
			
		}
		else{}
	}
	else{
		puts("IRQ_INVALID interrupt\r\n");
		return;
	}
}

unsigned long read_esr_el1() {
    unsigned long esr_el1;
    asm volatile("mrs %0, esr_el1" : "=r" (esr_el1));
    return esr_el1;
}

void sync_general_handler_c(trapframe_t* tf){
	//puts("in sync_general_handler\r\n");
	unsigned long esr_value=0;
	esr_value=read_esr_el1();
	esr_el1_t* esr=(esr_el1_t*)&esr_value;


	unsigned long ec=esr->ec;
	if(ec == EC_DATAABORT_LEL || ec == EC_DATAABORT_SEL){
		puts("data abort error:\r\n");
		puts("ec:");
		put_hex(ec);
		puts("\r\n");
		unsigned long long elr_el1 = 0;
		asm volatile("mrs %0, elr_el1":"=r"(elr_el1));
		puts("elr_el1: ");
		put_long_hex(elr_el1);
		puts("\r\n");
		unsigned long long far_el1 = 0;
		asm volatile("mrs %0, far_el1":"=r"(far_el1));
		puts("far_el1: ");
		put_long_hex(far_el1);
		puts("\r\n");
		puts("iss:");
		put_long_hex(esr->iss);
		puts("\r\n");
		reset(0x400);
	}
	else if(ec == EC_SVC){
		int syscall_number=tf->x8;
		if(syscall_number == 0){
			tf->x0=sys_getpid(tf);
			return;
		}
		else if(syscall_number == 1){
			tf->x0=sys_uart_read(tf);
			return;
		}
		else if(syscall_number == 2){
			tf->x0=sys_uart_write(tf);
			return;
		}
		else if(syscall_number == 3){
			tf->x0=sys_exec(tf);
			return;
		}
		else if(syscall_number == 4){
			sys_fork(tf);
			return;
		}
		else if(syscall_number == 5){
			tf->x0=sys_exit(tf);
			return;
		}
		else if(syscall_number == 6){
			sys_mbox_call(tf);
			return;
		}
		else if(syscall_number == 7){
			sys_kill(tf);
			return;
		}
		else{
			puts("error: wrong syscall number\r\n");
			reset(0x400);
		}
	}
	else{
		puts("error:unhandle sync exception\r\n");
		puts("ec:");
		put_hex(ec);
		puts("\r\n");
		unsigned long long elr_el1 = 0;
		asm volatile("mrs %0, elr_el1":"=r"(elr_el1));
		puts("elr_el1: ");
		put_long_hex(elr_el1);
		puts("\r\n");
		unsigned long long far_el1 = 0;
		asm volatile("mrs %0, far_el1":"=r"(far_el1));
		puts("far_el1: ");
		put_long_hex(far_el1);
		puts("\r\n");
		puts("iss:");
		put_long_hex(esr->iss);
		puts("\r\n");
		reset(0x400);

	}
	
}

void output_trapframe(trapframe_t* tf){
	unsigned long* arr=tf;
	for(int i=0;i<31;i++){
		puts("x");
		put_int(i);
		puts(":");
		put_hex(arr[i]);
		puts(" ");
	}
	puts(" spsr_el1:");
	put_hex(arr[31]);
	puts(" elr_el1:");
	put_hex(arr[32]);
	puts(" sp_el0:");
	put_hex(arr[33]);
	puts("\r\n");
	return;
}