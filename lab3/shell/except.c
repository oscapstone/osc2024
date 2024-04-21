
#include "header/uart.h"
#include "header/irq.h"
#include "header/shell.h"
#include "header/timer.h"
#include "header/tasklist.h"

#define CNTPSIRQ_BIT_POSITION 0x02
#define AUXINIT_BIT_POSTION 1<<29


// Basic1
void except_handler_c() {
	uart_send_string("In Exception handle\n");

	// EL1 configuration, spsr_el1[9:6]=4b0 to enable interrupt
	// spsr_el1 : Saved program status register
	unsigned long long spsr_el1 = 0;
	asm volatile("mrs %0, spsr_el1":"=r"(spsr_el1));
	uart_send_string("spsr_el1: ");
	uart_hex(spsr_el1);
	uart_send_string("\n");

	// ELR_EL1 holds the address if return to EL1
	unsigned long long elr_el1 = 0;
	asm volatile("mrs %0, elr_el1":"=r"(elr_el1));
	uart_send_string("elr_el1: ");
	uart_hex(elr_el1);
	uart_send_string("\n");
	
	// esr_el1 : exception syndrome reg(EL1)
	// ESR_EL1 holds symdrome information of exception, to know why the exception happens.
	// 任何發生在EL1的exception, 儲存syndrome exception
	unsigned long long esr_el1 = 0;
	asm volatile("mrs %0, esr_el1":"=r"(esr_el1));
	uart_send_string("esr_el1: ");
	uart_hex(esr_el1);
	uart_send_string("\n");


}


/*
// ec : exception class
// 表示reg儲存資訊的exception原因
// ec bits[31:26]可能的value, 0b010101
// 表示aarch64 state的svc指令執行
unsigned ec = (esr_el1 >> 26) & 0x3F; //0x3F = 0b111111(6)
uart_send_string("ec: ");
uart_hex(ec);
uart_send_string("\n");
*/


//timerlist
void timer_irq_handler() {
	//enable core_0_timer
	unsigned int* address = (unsigned int*) CORE0_TIMER_IRQ_CTRL;
	*address = 2;
	
	// Disable timer interrup, 因為發生timer interrupt
	asm volatile("msr cntp_ctl_el0,%0"::"r"(0));
	// Disable interrupts to protect critical section
	asm volatile("msr DAIFSet, 0xf");
	
	// 讀取timer reg取得current timer
	uint64_t current_time;
	asm volatile("mrs %0, cntpct_el0":"=r"(current_time));

	while(timer_head && timer_head->expiry <= current_time) {
		timer_t *timer = timer_head;

		//Execute the callback
		timer->callback(timer->data);

		// Remove timer from the list
		timer_head = timer->next;
		if (timer_head) {
		    timer_head->prev = NULL;
		}
		//如果timer是空或是新的
		if(timer_head) {
			asm volatile("msr cntp_cval_el0, %0"::"r"(timer_head->expiry));
			asm volatile("msr cntp_ctl_el0,%0"::"r"(1));
		} else {
			// 沒timer就mask timer interrupt
			asm volatile("msr cntp_ctl_el0,%0"::"r"(0));
		}
		//enable interrupt
		asm volatile("msr DAIFClr,0xf");
	}
}

//Basic3
void uart_transmit_handler() {
    // AUX_MU_IER : enable interrupts
    // 0x2 = 0x0010 -> enable transmit interrupt
    mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) | (0x2));

    // Send data from the write buffer
    while (uart_write_head != uart_write_index) {
        // 將buffer的data寫入 AUX_MU_IO，UART的output buffer
        mmio_write(AUX_MU_IO, uart_write_buffer[uart_write_head++]);
        if (uart_write_index >= UART_BUFFER_SIZE) {
            uart_write_index = 0;
        }
		
		
	if (uart_write_head == uart_write_index) {
	    // 沒data要送, diabled uart TX interrupt
            // Disable tx interrupt when there is no data left to send
	    // ~0x2 = 0x1101, 第 1 bit永遠設為0
	    mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) & ~0x2);
	    
            // buffer寫滿
	    if(uart_read_buffer[uart_read_index-1] == '\r'){
		uart_read_buffer[uart_read_index-1] = '\0';
		parse_command(uart_read_buffer);
		uart_read_index = 0;
		uart_write_index = 0;
		uart_write_head = 0;
	    }	
	}	
    } 
}

//Basic3
void uart_receive_handler() {
    // AUX_MU_IER : enable interrupts
    // 0x1 = 0x0001 -> enable receive interrupt
    mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) | 0x1);
    // Read data(8 bytes) and store it in the read buffer
    
    // spec p.11
    // Read data(8 bytes) and store it in the read buffer
    // AUX_MU_IO : write data to and read data from the UART FIFOs
    // AUX_MU_IO 0~7 bits transmit or receive data
    char data = mmio_read(AUX_MU_IO) & 0xff;
    uart_read_buffer[uart_read_index++] = data;
    if (uart_read_index >= UART_BUFFER_SIZE) {
        uart_read_index = 0;
    }

    // Enqueue the received data into the write buffer
    uart_write_buffer[uart_write_index++] = data;
    if (uart_write_index >= UART_BUFFER_SIZE) {
        uart_write_index = 0;
    }

    create_task(uart_transmit_handler,2);
}

/*void irq_except_handler_c() {
	
	uart_send_string("In timer interruption\n");

	unsigned long long cntpct_el0 = 0;//The register count secs with frequency
	asm volatile("mrs %0,cntpct_el0":"=r"(cntpct_el0));
	
	unsigned long long cntfrq_el0 = 0;//The base frequency
	asm volatile("mrs %0,cntfrq_el0":"=r"(cntfrq_el0));

	unsigned long long sec = cntpct_el0 / cntfrq_el0;
	uart_send_string("sec:");
	uart_hex(sec);
	uart_send_string("\n");
	
	unsigned long long wait = cntfrq_el0 * 2;// wait 2 seconds
	asm volatile ("msr cntp_tval_el0, %0"::"r"(wait));//set new timer
    // fulfill the requirement set the next timeout to 2 seconds later.
	
}*/






//timer, uart的read跟write都透過irq_except_handler()處理
//每發生一個task就加入至tasklist
//timer則是加在timer list裡
//當時間觸發到,就轉移加入到tasklist
//之後按照順序執行
//basic3 advance2
void irq_except_handler() {
    
    // Handle timeout interrupt
    // 設定setTimeout後，當發生interrupt時，timeout interrupt handler處理interrupt
    asm volatile("msr DAIFSet, 0xf"); // Disable interrupts
    //判斷是哪種interrupt
    uint32_t core0_interrupt_source = mmio_read(CORE0_INTERRUPT_SOURCE);
    uint32_t iir = mmio_read(AUX_MU_IIR);
    
    
    // Advance2
    // task interrupt
    if (core0_interrupt_source) {
	// 因為進入timer interrupt先關掉core0 timer
	// 等處理一個階段再打開
	// 避免中斷一半又有timer interrupt
	unsigned int* address = (unsigned int*) CORE0_TIMER_IRQ_CTRL;
	*address = 0;
	create_task(timer_irq_handler,3);
    }


    // Basic3
    // Handle UART interrupt 
    uint32_t irq_pending1 = mmio_read(IRQ_PENDING_1);
    if (irq_pending1 & AUXINIT_BIT_POSTION) {
        // AUX_MU_IIR : interrupt status
        // 0x04 = 0x0100 -> receive
        // Check if it is a receive interrupt
        if ((iir & 0x06) == 0x04) {
         
	    // Disable receive interrupt
	    // AUX_MU_IER : enable interrupts
	    // 0x01 = 0x0001 -> enable receive interrupt
	    mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) & ~(0x01));
	    create_task(uart_receive_handler, 1);
	}
	// 0x02 = 0x0010 -> transmit
        // Check if it is a transmit interrupt
	if ((iir & 0x06) == 0x02) {
	
	    // Disable transmit interrupt
	    // 0x2 = 0x0010 -> enable transmit interrupt
	    mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) & ~(0x02));
	    create_task(uart_transmit_handler, 2);
	}
    }
    asm volatile("msr DAIFClr, 0xf"); // Enable interrupts
    execute_tasks();
}




// Asynchronous UART Read and Write by interrupt handlers.
void uart_irq_handler(){
    // AUX_MU_IIR : interrupt status
    // spec p.13
    uint32_t iir = mmio_read(AUX_MU_IIR);
    // 0x06 = 0x0110 -> read or write
    // 0x04 = 0x0100 -> receive
    // Check if it is a receive interrupt
    if ((iir & 0x06) == 0x04) {
    
    
        // spec p.11
        // Read data(8 bytes) and store it in the read buffer
        // AUX_MU_IO : write data to and read data from the UART FIFOs
        // AUX_MU_IO 0~7 bits transmit or receive data
        char data = mmio_read(AUX_MU_IO) & 0xff;
        uart_read_buffer[uart_read_index++] = data;
        // buffer寫滿
        if (uart_read_index >= UART_BUFFER_SIZE) {
            uart_read_index = 0;
        }

        // Enqueue the received data into the write buffer
        uart_write_buffer[uart_write_index++] = data;
        if (uart_write_index >= UART_BUFFER_SIZE) {
            uart_write_index = 0;
        }
	
	// AUX_MU_IER : enable interrupts
	// 0x2 = 0x0010 -> enable transmit interrupt
	// transmit FIFO is empty
        // Enable tx interrupt
        mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) | 0x2);
    }
    
    // 0x02 = 0x0010 -> transmit
    // Check if it is a transmit interrupt
    if ((iir & 0x06) == 0x02) {
        // Send data from the write buffer
        // 還有data要傳送
        if (uart_write_head != uart_write_index) {
            // 將buffer的data寫入 AUX_MU_IO，UART的output buffer
            mmio_write(AUX_MU_IO, uart_write_buffer[uart_write_head++]);
            if (uart_write_index >= UART_BUFFER_SIZE) {
                uart_write_index = 0;
            }
        } else {
            // 沒data要送, diabled uart TX interrupt
            // Disable tx interrupt when there is no data left to send
	    // ~0x2 = 0x1101, 第 1 bit永遠設為0
            mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) & ~0x2);

            if(uart_read_buffer[uart_read_index-1] == '\r'){
                uart_read_buffer[uart_read_index-1] = '\0';
                parse_command(uart_read_buffer);
                uart_read_index = 0;
                uart_write_index = 0;
                uart_write_head = 0;
            }
        }
    }	
}
