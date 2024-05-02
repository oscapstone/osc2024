#define VSPRINT_MAX_BUF_SIZE 1024

char getchar();
void putchar(char c);
void put_int(int num);
void puts(const char *s);
void put_hex(unsigned int num);
unsigned int sprintf(char *dst, char *fmt, ...);
unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args);
void printf(char *fmt, ...);