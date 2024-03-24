void uart_init();
void uart_send(unsigned int c);
char uart_getc();
void uart_puts(char *s);
void uart_hex(unsigned int d);
int memcmp(void *s1, void *s2, int n); //from gh
void uart_puts_l(char *s, int l);