#include "shell.h"
#include "mini_uart.h"
#include "utils.h"
#include "mailbox.h"
#include "reboot.h"
#include "cpio.h"
#include "malloc.h"

#define MAX_BUFFER_SIZE 256u

static char buffer[MAX_BUFFER_SIZE];

int stringcmp(const char *p1, const char *p2)
{
    const unsigned char *s1 = (const unsigned char *) p1;
    const unsigned char *s2 = (const unsigned char *) p2;
    unsigned char c1, c2;
    
    c1 = (unsigned char) *s1;
    c2 = (unsigned char) *s2;
    while(c1 == c2) {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if(c1 == '\0') { return c1 - c2; }
    }

    return c1 - c2;
}

int str_to_int(char *str) {
    int val = 0;
    while(*str != '\0') {
        val = val*10 + (*str - '0');
        str++;
    }
    return val;
}

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

    if (stringcmp(buffer, "\0") == 0) 
        uart_send_string("\r\n");
    else if (stringcmp(buffer, "hello") == 0)
        uart_send_string("Hello World!\r\n");
    else if (stringcmp(buffer, "reboot") == 0) {
        uart_send_string("rebooting...\r\n");
        reset(100);
    }
    else if (stringcmp(buffer, "hwinfo") == 0) {
        get_board_revision();
        get_arm_memory();
    }
    else if (stringcmp(buffer, "help") == 0) {
        uart_send_string("help:\t\tprint list of available commands\r\n");
        uart_send_string("hello:\t\tprint Hello World!\r\n");
        uart_send_string("reboot:\t\treboot the device\r\n");
        uart_send_string("hwinfo:\t\tprint hardware information\r\n");
        uart_send_string("ls:\t\tlist out files in cpio archive\r\n");
        uart_send_string("cat:\t\tshow the content in the input file\r\n");
        uart_send_string("malloc:\t\tallocate a continuous space for requested size.\r\n");
    }
    else if (stringcmp(buffer, "ls") == 0) {
        my_ls();
        for(int i = 0; i < 1000; i++) { asm volatile("nop"); }
    }
    else if (stringcmp(buffer, "cat") == 0) {
        my_cat();
        for(int i = 0; i < 1000; i++) { asm volatile("nop"); }
    }
    else if (stringcmp(buffer, "malloc") == 0) {
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
    else {
        uart_send_string("Command not found! Type help for commands.\r\n");
    }

}

void shell() 
{
    while (1) {
        uart_send_string("$ ");
        read_cmd();
        parse_cmd();
    }
}
