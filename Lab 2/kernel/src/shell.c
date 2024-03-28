#include "type.h"
#include "uart.h"
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
    byteptr_t str = memory_alloc(size);
    for (uint32_t i = 0; i < size; ++i) {
        str[i] = i + '1';
    }
    str[size - 1] = '\0';
    uart_str("location = 0x");
    uart_hex((uint32_t) str); 
    uart_endl();
    uart_str("content  = \""); 
    uart_str(str); 
    uart_line("\"");
}


static void
print_help()
{
    uart_line("--------------------------------");
    uart_str("help\t| ");
    uart_line("print this help menu");
    uart_str("info\t| ");
    uart_line("print VideoCore info");
    uart_str("dtb\t| ");
    uart_line("print the device tree");
    uart_str("ls\t| ");
    uart_line("list directory contents");
    uart_str("cat\t| ");
    uart_line("print the file content");
    uart_str("malloc\t| ");
    uart_line("simple memory allocation demo");
    uart_str("reboot\t| ");
    uart_line("reboot this device");
    uart_line("--------------------------------");
}


static byteptr_t
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


static void 
parse_command(byteptr_t buffer)
{
    byteptr_t cmd = str_tok(buffer);

    if (str_eql(cmd, "help")) {
        print_help();
    }

    else if (str_eql(cmd, "reboot")) {
        uart_line("rebooting...");
        power_reset(100);
        uart_get();
        delay_cycles(500);
    }

    else if (str_eql(cmd, "ls")) {  
        initrd_list();
    }

    else if (str_eql(cmd, "cat")) {
        byteptr_t tok = str_tok(0);
        if (tok == nullptr) {
            uart_str("file: ");
            while (tok == nullptr) {
                read_command(buffer);
                tok = str_tok(buffer);
            }
        }
        initrd_cat(tok);
    }

    else if (str_eql(cmd, "info")) {  
        info_board_revision();
        info_memory();
        info_videocore();
    }

    else if (str_eql(cmd, "dtb")) {
        fdt_traverse(fdt_print_node);
    }

    else if (str_eql(cmd, "malloc")) {
        byteptr_t tok = str_tok(0);
        if (tok == nullptr) {
            uart_str("size: ");
            while (tok == nullptr) {
                read_command(buffer);
                tok = str_tok(buffer);
            }
        }
        uint32_t size = ascii_to_uint32(tok, str_len(tok));
        simple_malloc_demo(size);     
    }

    else {
        uart_str("not a command: ");
        uart_line(buffer);
    }
}


void 
shell() 
{
    while (1)
    {
        byte_t buffer[BUFFER_MAX_SIZE];
        uart_str("> ");
        read_command(buffer);
        parse_command(buffer);
    }
}



