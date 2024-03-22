#include "types.h"
#include "mini_uart.h"
#include "string.h"


#define BUFFER_MAX_SIZE     64


static const byteptr_t kernel_addr = (byteptr_t) 0x80000;

static void 
load_image(uint32_t len)
{
    byteptr_t cur_ptr = (byteptr_t) kernel_addr;
    while (len--)
    {
        *cur_ptr++ = mini_uart_getc();
        mini_uart_putc('#');
    }
    mini_uart_putln("\ndone.");
}


void 
print_help()
{
    mini_uart_puts("help\t| ");
    mini_uart_putln("print this help menu");
    mini_uart_puts("load\t| ");
    mini_uart_putln("wait and recieve the kernel image from uart");
    mini_uart_puts("boot\t| ");
    mini_uart_putln("jump to kernel");
}


byteptr_t
read_command(byteptr_t buffer)
{
    uint32_t index = 0;
    byte_t r = 0;
    do {
        r = mini_uart_getc();
        mini_uart_putc(r);
        buffer[index++] = r;
    } while (index < (BUFFER_MAX_SIZE - 1) && r != '\n');
    if (r == '\n') index--;
    buffer[index] = '\0';
    return buffer;
}

extern byteptr_t dtb_ptr;

void 
parse_command(byteptr_t buffer)
{
    if (str_eql(buffer, "help")) {
        print_help();
    }

    else if (str_eql(buffer, "load")) {
        mini_uart_puts("size = ");
        read_command(buffer);
        uint32_t len = ascii_dec_to_uint32(buffer, str_len(buffer));
        mini_uart_puts("loading");
        load_image(len);
    }

    else if (str_eql(buffer, "boot")) {
        mini_uart_putln("booting...");
        ((void (*)(uint64_t))kernel_addr)((uint64_t) dtb_ptr);
    }

    else {
        mini_uart_puts("not a command: ");
        mini_uart_putln(buffer);
    }
}


void shell() 
{
    while (1)
    {
        char buffer[BUFFER_MAX_SIZE];
        mini_uart_puts("$ ");
        read_command(buffer);
        parse_command(buffer);
    }
}



