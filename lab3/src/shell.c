#include "mini_uart.h"
#include "mbox.h"
#include "reboot.h"
#include "c_utils.h"
#include "string.h"
#include "initrd.h"

void shell(){
    uart_async_send_string("Welcome to OSC2024 shell!\n");
    while(1){
        uart_send_string("# ");
        char str[100];
        uart_recv_command(str);
        uart_send_string("\n");
        if(!strcmp(str, "hello")){
            uart_async_send_string("Hello World!\n");
        } else if(!strcmp(str, "mailbox")){
            uart_async_send_string("Mailbox info: \n");
            get_board_revision();
            get_memory_info();
        } else if(!strcmp(str, "ls")){
            initrd_list();
        } else if (!strcmp(str, "cat")){
            uart_async_send_string("Enter file name: ");
            char file_name[100];
            uart_recv_command(file_name);
            uart_async_send_string("\n");
            initrd_cat(file_name);
        } else if (!strcmp(str, "exec")){
            uart_async_send_string("Enter program name: ");
            char file_name[100];
            uart_recv_command(file_name);
            uart_async_send_string("\n");
            initrd_exec_prog(file_name);
        } else if (!strcmp(str, "count2s")){
            uart_async_send_string("Triggering timer interrupt...\n");
            asm volatile(
                "mov x20, 1;"
                "msr cntp_ctl_el0, x20;"
                "mrs x20, cntfrq_el0;"
                "msr cntp_tval_el0, x20;"
                "mov x20, 2;"
                "ldr x1, =0x40000040;"
                "str w20, [x1];"
            );
        } else if (!strcmp(str, "reboot")){
            uart_async_send_string("Rebooting...\n");
            reset(200);
            break;
        } else if(!strcmp(str, "help")){
            uart_async_send_string("help\t: print this help menu\n");
            uart_async_send_string("hello\t: print Hello World!\n");
            uart_async_send_string("mailbox\t: print board revision and memory info\n");
            uart_async_send_string("ls\t: list files in the ramdisk\n");
            uart_async_send_string("cat\t: print content of a file\n");
            uart_async_send_string("reboot\t: reboot this device\n");
            uart_async_send_string("exec\t: execute program from initramfs\n");
            uart_async_send_string("count2s\t: trigger timer interrupt\n");
        }
    }
}