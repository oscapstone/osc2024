#include "mini_uart.h"
#include"stdio.h"

char getchar()
{
    char c = uart_recv();
    return c == '\r' ? '\n' : c;
}

char getbyte(){
    char c = uart_recv();
    return c;
}

void get(char* arr,int len)
{	puts("# ");
	char c = '\0';
	int idx = 0;
	while(idx < len-1){
		c = getchar();
        	if (c == 127) // backspace
        	{
            		if (idx != 0)
            		{
                		puts("\b \b");
                		idx--;
            		}
        	}
        	else if (c == '\n')
        	{
            		break;
        	}
        	else if (c <= 16 || c >= 32 || c < 127)
        	{
            		putchar(c);
            		arr[idx++] = c;
        	}
	}
	arr[idx] = '\0';
	puts("\r\n");
}

void putchar(char c)
{
    uart_send(c);
}

void puts(const char *s)
{
    while (*s)
        putchar(*s++);
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
    while (index >= 0)
    {
        hex = (num >> index) & 0xF;
        hex += hex > 9 ? 0x37 : 0x30;
        putchar(hex);
        index -= 4;
    }
}

void put_long_hex(unsigned long long num){
    unsigned int hex;
    int index = 60;
    while (index >= 0)
    {
        hex = (num >> index) & 0xF;
        hex += hex > 9 ? 0x37 : 0x30;
        putchar(hex);
        index -= 4;
    }
}