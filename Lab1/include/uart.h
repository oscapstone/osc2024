void uart_init();
void uart_send(unsigned int c);
char uart_recv();
void uart_printf(char* fmt, ...);
void uart_flush();
void uart_puts(char *s);