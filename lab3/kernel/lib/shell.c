
#include "shell.h"
#include "uart.h"
#include "timer.h"

void cmd(char *s1) {
    
    char *token;
    char arg[20][50];
    
    for(int i = 0; i < 20; ++i)
        for(int j = 0; j < 50; ++j)
            arg[i][j] = '\0';
    
    int i = 0;
    token = strtok(s1, " ");

    while(token != 0) {
        strcpy(arg[i], token);
        //uart_puts(arg[i]);
        ++i;
        //uart_send('0'+i);
        token = strtok(0, " ");
    }
    
    if(!strcmp(arg[0], "help") && i == 1) {
        uart_puts("cat    \t: cat\n");
        uart_puts("clear  \t: clear all\n");
        uart_puts("exec   \t: execute test file\n");
        uart_puts("help   \t: print this help menu\n");
        uart_puts("hello  \t: print Hello World!\n");
        uart_puts("ls     \t: ls\n");
        uart_puts("mailbox\t: show infos of board revision and ARM memory\n");
        uart_puts("malloc \t: allocate string abc\n");
        uart_puts("reboot \t: reboot the device\n");
    }
    else if(!strcmp(arg[0], "hello") && i == 1) {
        uart_puts("Hello World!\n");
    }
    else if(!strcmp(arg[0], "mailbox") && i == 1) {
        mbox_info();
    }
    else if(!strcmp(arg[0], "reboot") && i == 1) {
        reset(1);
    }
    else if(!strcmp(arg[0], "cat") && i == 2) {
        
        cat(arg[1]);
        //uart_puts(arg[1]);
    }
    else if(!strcmp(arg[0], "clear") && i == 1) {
        uart_puts("\x1b[2J\x1b[H");
    }
    else if(!strcmp(arg[0], "ls") && i == 1) {
        ls(".");
        //uart_puts(arg[1]);
    }
    else if(!strcmp(arg[0], "malloc")) {
        uart_puts("Allocating string abc\n");
        char *st = (char*)smalloc(16);
        // uart_puts("test1\n");
        st[0] = 'a';
        st[1] = 'b';
        st[2] = 'c';
        st[3] = '\0';
        uart_printf("%s\n", st);
    }
    else if(!strcmp(arg[0], "exec") && i == 1) {
        execfile("./test");
    }
    else if (!strcmp(arg[0], "sto") && i == 3) {
        // setTimeout MESSAGE SECONDS
        //char *sto = (char *)smalloc(20);
        //strcpy(sto, arg[1]);
        uart_printf("'%s' Set Timeout : %d\n", arg[1], atoi(arg[2]) * get_clock_freq() + get_clock_tick());
        add_timer(uart_puts, arg[1], atoi(arg[2]) * get_clock_freq() + get_clock_tick());
    }
    else if (!strcmp(arg[0], "tsa") && i == 2) {
        // tsa  MESSAGE
        //uart_printf("%d\n", get_clock_time());
        //uart_printf("t1\n", get_clock_time());
        //char *tsa = (char *)smalloc(20);
        //strcpy(tsa, arg[1]);

        add_timer(two_second_alert, arg[1], (unsigned long long)((unsigned long long)2 * get_clock_freq() + get_clock_tick()));
        
    }
    else if (!strcmp(arg[0], "async") && i == 1) {
        char c = ' ';
        uart_puts("Press enter to exit\n");
        while(c != '\n' && c != '\r') {
            c = uart_async_getc();
            uart_async_putc(c);
        }
        uart_puts("\n");

    }
    else if(!strcmp(arg[0], "preempt") && i == 1) {
        test_preemption();
    }
    else {
        uart_puts("Unknown command: ");
        uart_puts(s1);
        uart_puts("\n");
    }
}

void shell() {
    
    while(1) {
        uart_puts("# ");

        int i = 0;
        char str[50] = {};
        char c = ' ';
        
        while( c != '\n') {
            c = uart_getc();

            if(c == '\n') {
                uart_puts("\n");
            }
            else {
                uart_send(c);
            }
            if(c == 0x08 || c == 0x7f && i > 0) {
                uart_send('\b');
                uart_send(' ');
                uart_send('\b');
                str[strlen(str) - 1] = '\0';
                --i;
            }
            
            if(c != '\n' && c != 0x08 && c != 0x7f)
                str[i++] = c;

        }
        cmd(str);

    }
}