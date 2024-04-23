#ifndef	_EXCEPTION_H_
#define	_EXCEPTION_H_

// https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf p16
#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int*)(0x40000060))
#define INTERRUPT_SOURCE_CNTPNSIRQ (1<<1)
#define INTERRUPT_SOURCE_GPU (1<<8)
#define IRQ_PENDING_1_AUX_INT (1<<29)

void print_currentEL();
void invalid_exception_router(); // exception_handler.S

void el1_interrupt_enable();
void el1h_irq_router();
void el0_sync_router();
void el0_irq_64_router();

void lock();
void unlock();


#endif /*_EXCEPTION_H_*/
