#include"header/shell.h"
#include"header/uart.h"
#include"header/utils.h"
#include"header/reboot.h"
#include"header/mailbox.h"
#include"header/cpio.h"
#include"header/malloc.h"
#include"header/timer.h"
#include"header/task.h"
void shell(){
    while (1)
    {
        char cmd[20];
        for(int i = 0;i < 20; i++){
            cmd[20] = '\0';
        }
        char *cur;
        char *s = "# ";
        uart_send_str(s);
        cur = cmd;
        char receive;
        while (1)
        {
            receive = uart_get_char();
            if(receive == '\n'){
                *cur = '\0';
                break;
            }
            else if(receive == 127){
                if(cur == cmd){
                    *cur = '\0';
                    continue;
                }
                *cur = '\0';
                cur--;
                uart_send_str("\b \b");
                continue;
            }
            else{
                *cur = receive;
                uart_send_char(receive);
                cur++;
            }
        }
        choose_opt(cmd);
    }
}
void choose_opt(char *cmd){
    char arg[20][20];
    for(int i = 0;i < 20;i++){
        for(int j = 0;j < 20;j++){
            arg[i][j] = '\0';
        }
    }
    char *tk = strtok(cmd," ");
    int i = 0;
    for(i = 0; tk != 0;i++){
        strcpy(arg[i],tk);
        tk = strtok(0," ");
    }
    if(string_compare(arg[0],"help")){
        uart_send_str("\nhelp\t\t:print this help menu\r\n");
        uart_send_str("hello\t\t:print Hello World!\r\n");
        uart_send_str("info\t\t:Get the hardware's information\r\n");
        uart_send_str("ls\t\t:list files in directory\r\n");
        uart_send_str("cat\t\t:cat [dir]\r\n");
        uart_send_str("clear\t\t:clear terminal\r\n");
        uart_send_str("reboot\t\t:reboot the device\r\n");
        uart_send_str("malloc\t\t:alloc string\r\n");
        uart_send_str("async\t\t:asynchronous uart I/O\r\n");
        uart_send_str("timeout\t\t:timeout [message] [seconds]\r\n");
        uart_send_str("preemtion\t\t: preemtion testing\r\n");
    }
    else if(string_compare(arg[0],"hello")){
        uart_send_str("\r\nHello World!\r\n");
    }
    else if(string_compare(arg[0],"info")){
        uart_send_str("\r\nInfo:\r\n");
        uart_send_str("Board Vision: ");
        get_board_revision();
        get_memory_info();

    }
    else if(string_compare(arg[0],"clear")){
        uart_send_str("\x1b[2J\x1b[H");
    }
    else if(string_compare(arg[0],"ls")){
        uart_send_str("\r\n");
        ls(".");
    }
    else if(string_compare(arg[0],"cat")){
        uart_send_str("\r\n");
        cat(arg[1]);
    }
    else if (string_compare(arg[0],"reboot")) {
        uart_send_str("\r\nRebooting....\r\n");
        reset(1000);
    }  
    else if (string_compare(arg[0],"malloc")){
        uart_send_str("\r\n");
        unsigned int size = (strlen(arg[1]) + 31) >> 5 << 5;
        char *string = simple_malloc(size);
        strcpy(string,arg[1]);
        uart_send_str(string);
        uart_send_str("\r\n");
    }
    else if (string_compare(arg[0],"exec")){
        uart_send_str("\r\nexecute file: ");
        uart_send_str(arg[1]);
        uart_send_str("...\r\n");
        execfile(arg[1]);
    }
    else if (string_compare(arg[0], "async")){
        uart_send_str("\r\nasync begin....\r\n");
        char c = 0;
        uart_clear_buffers();
        while (1) {
            c = uart_async_getc();
            if (c == 13 || c == 10) {
                break;
            }
            
            uart_async_putc(c);
        }
        uart_send_str("\r\n");
    }
    else if(string_compare(arg[0], "timeout")){
        if(i != 3){
            uart_send_str("\r\nusage\t:timeout [message] [seconds]\r\n");
        }
        else 
        {
            core_timer_interrupt_enable();
            unsigned long long cntpct = 0, cntfrq = 0;
            asm volatile("mrs %0, cntpct_el0\n\t" : "=r"(cntpct));
            asm volatile("mrs %0, cntfrq_el0\n\t" : "=r"(cntfrq));
            // add_timer(uart_send_str, atoi(arg[2])*cntfrq + cntpct, arg[1]);
            add_timer(uart_send_str, atoi(arg[2]), arg[1]);
        }
        uart_send_str("\r\n");
    }
    else if (string_compare(arg[0], "preempt")){
        uart_async_puts("\r\npreemption testing....\r\n");
        test_preempt();
        uart_async_puts("\r\n");    
    }
    else
        uart_send_str("\r\n");
}    
