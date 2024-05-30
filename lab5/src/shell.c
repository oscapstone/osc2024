#include "mini_uart.h"
#include "mbox.h"
#include "reboot.h"
#include "c_utils.h"
#include "string.h"
#include "initrd.h"
#include "timer.h"
#include "thread.h"
#include "alloc.h"
#include "exception.h"
#include "thread.h"

void shell(){
    uart_async_send_string("Welcome to OSC2024 shell!\n");
    while(1){
        uart_send_string("# ");
        char* str = kmalloc(100);
        uart_recv_command(str);
        uart_send_string("\n");
        if(!strcmp(str, "hello")){
            uart_async_send_string("Hello World!\n");
        } else if(!strcmp(str, "mailbox")){
            uart_send_string("Mailbox info: \n");
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
        } else if (!strcmp(str, "run")){
            initrd_run_syscall();
        }  else if (!strncmp(str, "setTimeout", 10)){
            // str = setTimeout MESSAGE SECONDS
            // split the message and seconds into two char*
            // without using lib

            // find the first space
            int i = 0;
            while(str[i] != ' '){
                i++;
            }
            // find the second space
            int j = i + 1;
            while(str[j] != ' '){
                j++;
            }
            // copy the message
            char message[100];
            for(int k = i + 1; k < j; k++){
                message[k - i - 1] = str[k];
            }
            message[j - i - 1] = '\0';
            // copy the seconds
            char seconds[100];
            for(int k = j + 1; k < strlen(str); k++){
                seconds[k - j - 1] = str[k];
            }
            seconds[strlen(str) - j - 1] = '\0';
            // convert seconds to int
            unsigned int timeout = atoi(seconds);
            uart_send_string(message);
            uart_send_string(" will be printed in ");
            uart_send_string(seconds);
            uart_send_string(" seconds\n");
            set_timeout(message, timeout);
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
            uart_async_send_string("setTimeout\t: setTimeout MESSAGE SECONDS\n");
        }
    }
    thread_exit();
}