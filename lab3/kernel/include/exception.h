#ifndef	_EXCEPTION_H_
#define	_EXCEPTION_H_

#define UART_IRQ_PRIORITY  1

//https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf p16
#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int*)(0x40000060))

#define INTERRUPT_SOURCE_CNTPNSIRQ (1<<1)
void el0_sync_router();
void el0_irq_64_router();

void invalid_exception_router(); // exception_handler.S

#endif /*_EXCEPTION_H_*/
