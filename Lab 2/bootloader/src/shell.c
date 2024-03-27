#include "type.h"
#include "delay.h"
#include "uart.h"
#include "string.h"

#define BUFFER_MAX_SIZE     64


static const byteptr_t kernel_addr = (byteptr_t) 0x80000;
extern byteptr_t g_dtb_ptr;
extern uint32_t  g_kernel_loaded;


static void 
load_image(uint32_t len)
{
    byteptr_t cur_ptr = (byteptr_t) kernel_addr;
    while (len--)
    {
        *cur_ptr++ = uart_get();
    }
    uart_line("\n$done$");
    g_kernel_loaded = 1;
}


void 
print_help()
{
    uart_line("--------------------------------");
    uart_str("help\t| ");
    uart_line("print this help menu");
    uart_str("load\t| ");
    uart_line("wait and recieve the kernel image from uart");
    uart_str("boot\t| ");
    uart_line("jump to kernel");
    uart_line("--------------------------------");
}


byteptr_t
read_command(byteptr_t buffer)
{
    uint32_t index = 0;
    byte_t r = 0;
    do {
        r = uart_getc();
        if (r == '\n') uart_put('\r');
        if (r == 0x7f) {
            index = (index > 0) ? index - 1 : 0;
            uart_put(0x8);
            uart_put(r);
        } else {
            buffer[index++] = r;
            uart_put(r);
        } 
    } while (index < (BUFFER_MAX_SIZE - 1) && r != '\n');
    if (r == '\n') index--;
    buffer[index] = '\0';
    return buffer;
}


void 
parse_command(byteptr_t buffer)
{
    if (str_eql(buffer, "help")) {
        print_help();
    }

    else if (str_eql(buffer, "load")) {
        uart_line("$size$");
        read_command(buffer);
        uint32_t len = ascii_dec_to_uint32(buffer);
        uart_line("$loading$");
        load_image(len);
    }

    else if (str_eql(buffer, "boot")) {
        if (g_kernel_loaded) {
            uart_put('\r');
            delay_cycles(200);
            ((void (*)(uint64_t))kernel_addr)((uint64_t) g_dtb_ptr);
        }
        uart_line("kernel is not loaded.");
    }

    else {
        uart_str("not a command: ");
        uart_line(buffer);
    }
}


void shell() 
{
    while (1)
    {
        char buffer[BUFFER_MAX_SIZE];
        uart_str("> ");
        read_command(buffer);
        parse_command(buffer);
    }
}



