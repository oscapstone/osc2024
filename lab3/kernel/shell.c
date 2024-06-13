#include "shell.h"
#include "mini_uart.h"
#include "utils.h"
#include "mailbox.h"
#include "reboot.h"
#include "cpio.h"
#include "malloc.h"
#include "str_util.h"
#include "timer.h"
#include "peripherals/base.h"
#include "exception.h"

#define MAX_BUFFER_SIZE 256u

extern void from_el1_to_el0(unsigned long prog_addr, unsigned long stack_top);

static char buffer[MAX_BUFFER_SIZE];

extern void async_uart_send_string(char *str);
extern void async_uart_init();
extern void test_async();

void read_cmd()
{
    unsigned int idx = 0;
    char c = '\0';
    
    while (1) {
        c = uart_recv();
        if (c == '\r' || c == '\n') {
            uart_send_string("\r\n");
            
            if (idx < MAX_BUFFER_SIZE) buffer[idx] = '\0';
            else buffer[MAX_BUFFER_SIZE-1] = '\0';
            
            break;
        } 
        else {
            uart_send(c);
            buffer[idx++] = c;
        } 
    }
}

void parse_cmd()
{

    if (str_cmp(buffer, "\0") == 0) 
        uart_send_string("\r\n");
    else if (str_cmp(buffer, "hello") == 0)
        uart_send_string("Hello World!\r\n");
    else if (str_cmp(buffer, "reboot") == 0) {
        uart_send_string("rebooting...\r\n");
        reset(100);
    }
    else if (str_cmp(buffer, "hwinfo") == 0) {
        get_board_revision();
        get_arm_memory();
    }
    else if (str_cmp(buffer, "help") == 0) {
        uart_send_string("help:\t\tprint list of available commands\r\n");
        uart_send_string("hello:\t\tprint Hello World!\r\n");
        uart_send_string("reboot:\t\treboot the device\r\n");
        uart_send_string("hwinfo:\t\tprint hardware information\r\n");
        uart_send_string("ls:\t\tlist out files in cpio archive\r\n");
        uart_send_string("cat:\t\tshow the content in the input file\r\n");
        uart_send_string("malloc:\t\tallocate a continuous space for requested size.\r\n");
        uart_send_string("exec:\t\texecute user program\r\n");
        uart_send_string("async_rw:\t\tasynchronous uart demo\r\n");
        uart_send_string("setTimeout [msg] [sec]:\t\ttimer multiplexing demo\r\n");
    }
    else if (str_cmp(buffer, "ls") == 0) {
        my_ls();
        for(int i = 0; i < 1000; i++) { asm volatile("nop"); }
    }
    else if (str_cmp(buffer, "cat") == 0) {
        my_cat();
        for(int i = 0; i < 1000; i++) { asm volatile("nop"); }
    }
    else if (str_cmp(buffer, "malloc") == 0) {
        char asize_i[100];
        unsigned int idx = 0;
        char c = '\0';
        int msize;
        uart_send_string("Input decimal allocate size: ");
        while (1) {
            c = uart_recv();
            if (c == '\r' || c == '\n') {
                uart_send_string("\r\n");
                
                if (idx < 100) asize_i[idx] = '\0';
                else asize_i[99] = '\0';
                
                break;
            } 
            else {
                uart_send(c);
                asize_i[idx++] = c;
            } 
        }
        msize = str_to_int(asize_i);
        char *ptr = my_malloc(msize);
        for(int i = 0; i < msize - 1; i++) {
            *(ptr+i) = 'a' + i;
        }
        *(ptr + msize - 1) = '\0';
        uart_send_string(ptr);
        uart_send_string("\r\n");
    }
    else if (str_cmp(buffer, "exec") == 0) {
        void *stack_top = (void*)my_malloc(0x1000); // 4096
        uart_send_string("Stack top of user program = 0x");
        uart_send_hex((unsigned long)stack_top);
        uart_send_string("\r\n");
        void *prog_addr = load_usr_prog("user_program.img");
        core_timer_enable();

        from_el1_to_el0( (unsigned long)prog_addr, (unsigned long)stack_top );
    }
    else if (str_cmp(buffer, "async_rw") == 0) {
        from_el1_to_el0( (unsigned long)test_async, 0x120000 );
    }
    else if (str_cmp_len(buffer, "setTimeout", 10) == 0) {
        enable_el1_interrupt();
        timer_multiplex(buffer);
    }
    else {
        uart_send_string("Command not found! Type help for commands.\r\n");
    }

}

void shell() 
{
    timer_queue_ini();
    while (1) {
        uart_send_string("$ ");
        read_cmd();
        parse_cmd();
    }
}
