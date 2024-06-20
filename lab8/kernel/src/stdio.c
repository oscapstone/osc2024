#include "stdio.h"
#include "uart1.h"
#include "vfs.h"
#include "sched.h"

extern thread_t *curr_thread;

char getchar()
{
    char c[1];
    stdio_op(stdin, c, 1);
    return c[0] == '\r' ? '\n' : c[0];
}

void putchar(char c)
{
    char cp[2] = {c,'\0'};
    stdio_op(stdout, cp, 1);
}

void puts(const char *s)
{
    while (*s)
        putchar(*s++);
}

void Readfile(char *str, int size)
{
    while (size--)
    {
        putchar(*str++);
    }
}

// Function to print an integer to the UART
void put_int(int num)
{
    // Handle the case when the number is 0
    if (num == 0)
    {
        putchar('0');
        return;
    }

    // Temporary array to store the reversed digits as characters
    char temp[12]; // Assuming int can have at most 10 digits
    int idx = 0;

    // Handle negative numbers
    if (num < 0)
    {
        putchar('-');
        num = -num;
    }

    // Convert the number to characters and store in the temporary array in reverse order
    while (num > 0)
    {
        temp[idx++] = (char)(num % 10 + '0');
        num /= 10;
    }

    // Reverse output the character digits
    while (idx > 0)
    {
        putchar(temp[--idx]);
    }
}

void put_hex(unsigned int num)
{
    unsigned int hex;
    int index = 28;
    puts("0x");
    while (index >= 0)
    {
        hex = (num >> index) & 0xF;
        hex += hex > 9 ? 0x37 : 0x30;
        putchar(hex);
        index -= 4;
    }
}

// A simple atoi() function
int atoi(char *str)
{
    // Initialize result
    int res = 0;

    // Iterate through all characters
    // of input string and update result
    // take ASCII character of corresponding digit and
    // subtract the code from '0' to get numerical
    // value and multiply res by 10 to shuffle
    // digits left to update running total
    for (int i = 0; str[i] != '\0'; ++i)
    {
        if (str[i] > '9' || str[i] < '0')
            return res;
        res = res * 10 + str[i] - '0';
    }

    // return result.
    return res;
}

int fake_log2(unsigned long long n)
{
    int val = 0;
    while (n >>= 1)
        val++;
    return val;
}

void delay(int r)
{
    while (r--)
    {
        asm volatile("nop");
    }
}
