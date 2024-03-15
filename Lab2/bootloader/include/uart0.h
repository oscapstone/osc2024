void uart_init();
void uart_send(unsigned int c);
char uart_recv();
char uart_recv_raw();
void uart_printf(char* fmt, ...);
void uart_flush();
void uart_puts(char *s);