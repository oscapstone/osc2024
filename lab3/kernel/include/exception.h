#ifndef	_EXCEPTION_H_
#define	_EXCEPTION_H_

void invalid_exception_router(); // exception_handler.S

void el1h_irq_router();
void el0_sync_router();
void el0_irq_64_router();

#endif /*_EXCEPTION_H_*/
