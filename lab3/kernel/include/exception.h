#ifndef	_EXCEPTION_H_
#define	_EXCEPTION_H_
#define UART_IRQ_PRIORITY  1
void el0_sync_router();

void invalid_exception_router(); // exception_handler.S

#endif /*_EXCEPTION_H_*/
