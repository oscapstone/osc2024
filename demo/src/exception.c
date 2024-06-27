#include "../include/exception.h"

extern char read_buffer[BUFFER_SIZE];
extern char write_buffer[BUFFER_SIZE];
extern int read_index_cur;
extern int read_index_tail;
extern int write_index_cur;
extern int write_index_tail;

extern int core_timer_flag;

void enable_interrupt(void){
    //write_register(daifclr,0xf);  //fail
    asm volatile ("msr daifclr, 0xf");
}

void disable_interrupt(void){
    //write_register(DAIFSet,0xf);   //fail
    asm volatile ("msr daifset, 0xf");
}


void showinfo_exception_handler(void) {
    
    disable_interrupt();
    

	uint64_t spsr, elr, esr, el;
    spsr = read_register(spsr_el1);
    elr = read_register(elr_el1);
    esr = read_register(esr_el1);
    el = read_register(CurrentEL); 

	uart_puts("Current exception level: ");
	uart_hex(el>>2);     // first two bits are preserved bit.
	uart_puts("\n");
	uart_puts("spsr_el1: 0x");
	uart_hex(spsr);
	uart_puts("\r\n");
	uart_puts("elr_el1: 0x");
	uart_hex(elr);
	uart_puts("\r\n");
	uart_puts("esr_el1: 0x");
	uart_hex(esr);
	uart_puts("\r\n");
    
    enable_interrupt();
}

void irq_exception_handler(void) {

	disable_interrupt();

	uint32_t cpu_irq_src, uart_irq_src, iir;

    //CORE0_INT_SRC provides information about the interrupt sources, specific to Core 0 
    //bit 2 for interrupt request 
	cpu_irq_src = *((volatile unsigned int*)CORE0_INT_SRC);

    //IRQ_PENDING_1  indicates which interrupts are pending, System-wide, covers all interrupt sources across the entire system.
    //bit 29 for UART interrupt 
	uart_irq_src = *((volatile unsigned int*)IRQ_PENDING_1);

    iir = *((volatile unsigned int*)AUX_MU_IIR);

    
    
	//uart_puts("In to Interrrupt\n");

	// Handle timer interrupt
	if(cpu_irq_src & (0x2)){
		
        //uart_puts("Timer Interrrupt\n");
		if (core_timer_flag == 1){
            //uart_puts("In to core timer \n");
			//core_timer_handler();
            task_t* newtask = create_task(core_timer_handler, 1);
            add_task_to_queue(newtask);

		}
		else{
            //uart_puts("In to system timer \n");
            // disnable core0 timer interrupt, remove will lead to core0 timer infinite interrupt
            *((volatile unsigned int*)CORE0_TIMER_IRQ_CTRL) = (0x0);
			//timer_interrupt_handler();
            task_t* newtask = create_task(timer_interrupt_handler, 1);
            add_task_to_queue(newtask);
		}
		
	}


    
    //uart_hex(gpu_irq_src & (1 << AUX_IRQ));
	// Handle UART interrupt
	if (uart_irq_src & (1 << AUX_IRQ)) {
        //uart_puts("Uart Interrrupt\n");
        // Check if it is a TX or RX interrupt
        // [2:1]=10 : Receiver holds valid byte 
        if (iir & 0x4) {
            // disable receive interrupt 
            *((volatile unsigned int*)AUX_MU_IER) &= ~(0x1);
            //uart_puts("in rx handler\n");
            task_t* newtask = create_task(uart_rx_handler, 1);
            add_task_to_queue(newtask);

        // [2:1]=01 : Transmit holding register empty
        } else if (iir & 0x2) {
            // disable transmit interrupt
            *((volatile unsigned int*)AUX_MU_IER) &= ~(0x2);
            //uart_puts("in tx handler\n");
            task_t* newtask = create_task(uart_tx_handler, 0);
            add_task_to_queue(newtask);
        }
    }

    enable_interrupt();

    ExecTasks();

    
}

void core_timer_handler(void){
    
    uint64_t cur_cnt, cnt_freq;
    cur_cnt = read_register(cntpct_el0);
    cnt_freq = read_register(cntfrq_el0);


    uart_puts("\nTime after boots: ");
    uart_hex(cur_cnt / cnt_freq);
    uart_puts(" sec.\n");
    uart_puts("# ");
    cnt_freq *= 2;
    write_register(cntp_tval_el0,cnt_freq);
    *((volatile unsigned int*)CORE0_TIMER_IRQ_CTRL) = (0x2);
}



//AUX_MU_IER 0x1 for reciev 0x2 for transmit

//reciev data
void uart_rx_handler(void) {
    //uart_puts("in rx_handler\n");
    // wait until something is in the buffer
    while ( ! ( (*AUX_MU_LSR) & 0x01 ) );

	char c = ( char )( *AUX_MU_IO );
    //c ='\r'?'\n':c;
    //uart_send_async(c);
	read_buffer[read_index_tail++] = c;
	write_buffer[write_index_tail++] = c;

    // Enable mini UART transmiter interrupt
    *((volatile unsigned int*)AUX_MU_IER) |= (0x2);

}

//transmit data
void uart_tx_handler(void) {

    char check;
	while(write_index_cur != write_index_tail){

        char c = write_buffer[write_index_cur++];
        check = c;

        while(!((*AUX_MU_LSR) & 0x20) ){
            // if bit 5 is set, break and return IO_REG
            asm volatile("nop");
        }
        *AUX_MU_IO = c;
        if(c == '\n'){
            break;
        }
    }

    // If there is more data to send, ensure the interrupt is enabled
    if (write_index_cur != write_index_tail) {
        *((volatile unsigned int*)AUX_MU_IER) |= 0x02;
    } else {
        // Disable transmit interrupt if no more data to send
        *((volatile unsigned int*)AUX_MU_IER) &= ~0x02;
        //if(check != '\n'){
            //int time=150;
            //while(time--);
        //*((volatile unsigned int*)AUX_MU_IER) |= (0x1);
        //}
            
    }
    // enable mini UART receiver interrupt
	//*((volatile unsigned int*)AUX_MU_IER) |= (0x1);

}

void timer_interrupt_handler(void) {
    unsigned long long cur_cnt;
    task_timer_t *cur = timer_head;

    if(cur==0){
        return;
    }
        
    disable_interrupt();

    cur_cnt = read_register(cntpct_el0);
    //Processing Expired Timers
    while(cur_cnt >= cur->deadline){
        
        cur->callback(cur->data);
        
        timer_head = cur->next;

        //if list not empty
        if(timer_head != 0){
    
            timer_head->prev = 0;
            write_register(cntp_cval_el0,timer_head->deadline);
            // enable timer
            write_register(cntp_ctl_el0,1);
        
            //asd turn on core0 timer interrupt
			*((volatile unsigned int*)CORE0_TIMER_IRQ_CTRL) = (0x2);
        }
        else{
            uart_puts("Finish all timer\n");
			uart_puts("# ");

            // disable timer
            write_register(cntp_ctl_el0,0);

            break;
        }
        cur = cur->next;
    }
    
    enable_interrupt();

}

