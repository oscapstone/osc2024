#ifndef EXCEPTION_HDLR_H
#define EXCEPTION_HDLR_H

extern int test_NI;
void int_off(void);
void int_on(void);
void c_exception_handler();
void c_core_timer_handler();
void c_write_handler();
void c_recv_handler();
void c_timer_handler();

#endif