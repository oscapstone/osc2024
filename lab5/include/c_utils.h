#ifndef _C_UTILS_H
#define _C_UTILS_H

void uart_recv_command(char *str);
int align4(int n);
int atoi(const char *s);
unsigned int endian_big2little(unsigned int x);
void delay (unsigned int loop);

#endif // _C_UTILS_H