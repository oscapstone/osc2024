
#include "bootloader.h"
#include "mini_uart.h"

int str_to_int(const char *val_str) {
    int val = 0;
    while (*val_str >= '0' && *val_str <= '9') {
        val = val * 10 + (*val_str - '0');
        val_str++;
    }

    return val;
}

char *int_2_str(unsigned int val) 
{
    int i = 0;
    char *val_str = "\0";
    while (val != 0) 
    {
        int rem = val % 10;
        val_str[i++] = rem + '0';
        val = val / 10;
    }

    int start = 0;
    int end = i - 1;
    while (start < end) {
        char tmp = val_str[start];
        val_str[start] = val_str[end];
        val_str[end] = tmp;
        start++;
        end--;
    }

    val_str[i] = '\0';
    return val_str;
}

void load_kernel_img()
{
    char *size_buffer = "\0";
    char c;
    int idx = 0;

    while((c = uart_recv()) != '\r')
    {    
        if(c >= 48 && c <= 57)
            size_buffer[idx++] = c;
        else if (c == '\n')
            break;
        else
            continue;
    }
    size_buffer[idx] = '\0';
    for (int i = 0; i < idx; i++)
    {
        uart_send(size_buffer[i]);
    }
    uart_send_string("\r\n");

	unsigned int size = (unsigned int)str_to_int((const char *)size_buffer);
    uart_send_string("Kernel image size: ");
    uart_send_string(int_2_str(size));
    uart_send_string("\n");

    char *kerne_addr = (char *) 0x80000;
    unsigned int cnt = 0;
    while (size--)
    {
        *kerne_addr++ = uart_recv();
        cnt++;
        if(cnt % 100 == 0)
        {
            uart_send_string("Now received ");
            uart_send_string(int_2_str(cnt));
            uart_send_string(" char.\r\n");
        }
    } 

    uart_send_string("Finish receiving kernel image!\r\n");
    uart_send_string("Loading kernel image...\r\n");

    // set the return address (x30 register) to 0x80000
    // 0x80000 = kernel image entry point
    // and return to 0x80000
    asm volatile(
        "mov x0, x10;"
        "mov x30, 0x80000;"
        "ret;"
    );
}
