#include "type.h"
#include "uart.h"
#include "string.h"
#include "initrd.h"
#include "power.h"
#include "info.h"
#include "memory.h"
#include "dtb.h"
#include "delay.h"
#include "sched.h"
#include "core_timer.h"
#include "thread.h"


#define BUFFER_MAX_SIZE     64


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

    uart_str("exe\t| ");
    uart_line("execute a user program");

    uart_str("async\t| ");
    uart_line("async uart demo");

    uart_str("timer\t| ");
    uart_line("set timeout message [timer msg duration]");

    uart_str("malloc\t| ");
    uart_line("memory allocation [data size in bytes]");

    uart_str("free\t| ");
    uart_line("memory release [block addr in hexdecimal]");   

    uart_str("memory\t| ");
    uart_line("memory infomation");

    uart_str("thread\t| ");
    uart_line("thread test demo");

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
        // uart_printf("\n0x%x\n", r);
        if (r == '\n') {
            uart_put('\n');
            uart_put('\r');
        } 
        else if (r == 0x7f && index > 0) {
            index--;
            uart_put(0x8);
            uart_put(r);
        } 
        else if (r < 0x7f && (r < 0x11 || r > 0x1f)) {
            buffer[index++] = r;
            uart_put(r);
        } 
    } while (index < (BUFFER_MAX_SIZE - 1) && r != '\n');
    buffer[index] = '\0';
    return buffer;
}


static byteptr_t 
next_tok(byteptr_t buffer, byteptr_t message)
{
    byteptr_t tok = str_tok(0);
    if (tok == nullptr) {
        uart_str(message);
        while (tok == nullptr) {
            read_command(buffer);
            tok = str_tok(buffer);
        }
    }
    return tok;
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
        byteptr_t tok = next_tok(buffer, "file: ");
        initrd_cat(tok);
    }

    else if (str_eql(cmd, "info")) {  
        info_board_revision();
        info_memory();
        // info_videocore();
    }

    else if (str_eql(cmd, "dtb")) {
        fdt_traverse(fdt_print_node);
    }

    else if (str_eql(cmd, "exe")) {
        byteptr_t tok = next_tok(buffer, "file: ");
        initrd_exec(tok);
    }

    else if (str_eql(cmd, "async")) {
        mini_uart_async_demo();
    }

    else if (str_eql(cmd, "timer")) {
        byteptr_t tok = next_tok(buffer, "message: ");
        byte_t msg[32]; str_ncpy(msg, tok, 32);
        tok = next_tok(buffer, "duration: ");
        uint32_t duration = ascii_dec_to_uint32(tok);
        core_timer_add_timeout_event(msg, duration);
    }

    else if (str_eql(cmd, "malloc")) {
        byteptr_t tok = next_tok(buffer, "size: ");
        uint32_t size = ascii_dec_to_uint32(tok);
        byteptr_t ptr = kmemory_alloc(size);
        uart_printf("malloc: %d bytes, addr: 0x%x\n", size, ptr);
    }

    else if (str_eql(cmd, "free")) {
        byteptr_t tok = next_tok(buffer, "addr: ");
        uint64_t addr = ascii_to_uint64(tok, str_len(tok));
        uart_printf("free: %s, 0x%x\n", tok, addr);
        kmemory_release((byteptr_t) addr);
    }

    else if (str_eql(cmd, "memory")) {
        memory_print_info();
    }

    else if (str_eql(cmd, "thread")) {
        thread_demo();
    }

    else if (str_len(buffer) > 0) {
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



