#include "types.h"
#include "mini_uart.h"
#include "string.h"
#include "initrd.h"
#include "power.h"
#include "info.h"
#include "memory.h"


#define BUFFER_MAX_SIZE     64

static void
simple_malloc_demo(uint32_t size)
{
    size = (size > 9) ? 9 : size;
    byteptr_t str = simple_malloc(size);

    for (uint32_t i = 0; i < size; ++i) {
        str[i] = i + '0';
    }
    str[size - 1] = '\0';
    mini_uart_putln(str);
}


static void
print_help()
{
    mini_uart_puts("help\t| ");
    mini_uart_putln("print this help menu");
    mini_uart_puts("info\t| ");
    mini_uart_putln("print this hardware info");
    mini_uart_puts("dtb\t| ");
    mini_uart_putln("print the device tree");
    mini_uart_puts("ls\t| ");
    mini_uart_putln("list directory contents");
    mini_uart_puts("cat\t| ");
    mini_uart_putln("print the file contents");
    mini_uart_puts("malloc\t| ");
    mini_uart_putln("simple memory allocation demo");
    mini_uart_puts("reboot\t| ");
    mini_uart_putln("reboot this device");
}


static byteptr_t
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


static void 
parse_command(byteptr_t buffer)
{
    if (str_eql(buffer, "help")) {
        print_help();
    }

    else if (str_eql(buffer, "reboot")) {
        mini_uart_putln("rebooting...");
        power_reset(1000);
    }


    else if (str_eql(buffer, "ls")) {  
        initrd_list();
    }

    else if (str_eql(buffer, "cat")) {
        byteptr_t tok = str_tok(0);
        if (tok == nullptr) {
            mini_uart_puts("file: ");
            while (tok == nullptr) {
                read_command(buffer);
                tok = str_tok(buffer);
            }
        }
        initrd_cat(tok);
    }

    else if (str_eql(buffer, "info")) {  
        info_board_revision();
        info_memory();
        info_videocore();
    }

    else if (str_eql(buffer, "dtb")) {
        fdt_traverse(fdt_print_node);
    }

    else if (str_eql(buffer, "malloc")) {
        byteptr_t tok = str_tok(0);
        if (tok == nullptr) {
            mini_uart_puts("size: ");
            while (tok == nullptr) {
                read_command(buffer);
                tok = str_tok(buffer);
            }
        }
        uint32_t size = ascii_to_uint32(tok, str_len(tok));
        simple_malloc_demo(size);     
    }

    else {
        mini_uart_puts("not a command: ");
        mini_uart_putln(buffer);
    }
}


void 
shell() 
{
    while (1)
    {
        byte_t buffer[BUFFER_MAX_SIZE];
        mini_uart_puts("> ");
        read_command(buffer);
        byteptr_t cmd = str_tok(buffer);
        parse_command(cmd);
    }
}



