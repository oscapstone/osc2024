#include "sched.h"
#include "vfs.h"
extern thread_t *curr_thread;

#define stdio_op(fd, c, len)                                          \
    switch (fd)                                                       \
    {                                                                 \
    case (stdin):                                                     \
        (vfs_read(curr_thread->file_descriptors_table[fd], c, len));  \
        break;                                                        \
    case (stdout):                                                    \
        (vfs_write(curr_thread->file_descriptors_table[fd], c, len)); \
        break;                                                        \
    case (stderr):                                                    \
        (vfs_write(curr_thread->file_descriptors_table[fd], c, len)); \
        break;                                                        \
    default:                                                          \
        break;                                                        \
    }

char getchar();
void putchar(char c);
void put_int(int num);
void puts(const char *s);
void put_hex(unsigned int num);
int atoi(char *str);
void Readfile(char *str, int size);
int fake_log2(unsigned long long n);
void delay(int s);