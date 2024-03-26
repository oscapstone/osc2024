#include "type.h"
#include "mini_uart.h"
#include "string.h"
#include "initrd.h"
#include "power.h"
#include "info.h"
#include "memory.h"
#include "dtb.h"
#include "delay.h"


#define BUFFER_MAX_SIZE     64

static void
simple_malloc_demo(uint32_t size)
{
    size = (size > 8) ? 8 : size;
    byteptr_t str = simple_malloc(size);
    for (uint32_t i = 0; i < size; ++i) {
        str[i] = i + '1';
    }
    str[size - 1] = '\0';
    mini_uart_puts("location = 0x");
    mini_uart_hex((uint32_t) str); 
    mini_uart_endl();
    mini_uart_puts("content  = \""); 
    mini_uart_puts(str); 
    mini_uart_putln("\"");
}


static void
print_help()
{
    mini_uart_puts("help\t| ");
    mini_uart_putln("print this help menu");
    mini_uart_puts("info\t| ");
    mini_uart_putln("print VideoCore info");
    mini_uart_puts("dtb\t| ");
    mini_uart_putln("print the device tree");
    mini_uart_puts("ls\t| ");
    mini_uart_putln("list directory contents");
    mini_uart_puts("cat\t| ");
    mini_uart_putln("print the file content");
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
        if (r == '\n') mini_uart_putc('\r');
        if (r == 0x7f) {
            index = (index > 0) ? index - 1 : 0;
            mini_uart_putc(0x8);
            mini_uart_putc(r);
        } else {
            buffer[index++] = r;
            mini_uart_putc(r);
        } 
    } while (index < (BUFFER_MAX_SIZE - 1) && r != '\n');
    if (r == '\n') index--;
    buffer[index] = '\0';
    return buffer;
}


static uint32_t 
parse_command(byteptr_t buffer)
{
    if (str_eql(buffer, "help")) {
        print_help();
    }

    else if (str_eql(buffer, "reboot")) {
        mini_uart_putln("rebooting...");
        power_reset(1000);
        return 1;
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

    return 0;
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
        uint32_t end = parse_command(cmd);
        if (end) return;
    }
}



