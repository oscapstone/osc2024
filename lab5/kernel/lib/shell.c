
#include "shell.h"
#include "uart.h"
#include "timer.h"
#include "mm.h"

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
        uart_async_printf("cat    \t\t\t: cat\n");
        uart_async_printf("clear  \t\t\t: clear all\n");
        uart_async_printf("exec   \t\t\t: execute test file\n");
        uart_async_printf("help   \t\t\t: print this help menu\n");
        uart_async_printf("hello  \t\t\t: print Hello World!\n");
        uart_async_printf("ls     \t\t\t: ls\n");
        uart_async_printf("mailbox\t\t\t: show infos of board revision and ARM memory\n");
        uart_async_printf("malloc \t\t\t: allocate string abc\n");
        uart_async_printf("reboot \t\t\t: reboot the device\n");
        uart_async_printf("sto [MESSAGE] [SECONDS]\t: print message after [SECONDS] seconds\n");
        uart_async_printf("tsa [MESSAGE] \t\t: set an alarm that alert every two seconds\n");
        uart_async_printf("preempt \t\t: test preemption\n");
        uart_async_printf("pfa \t\t\t: test page frame allocator\n");
        uart_async_printf("csa \t\t\t: test chunk slot allocator\n");
        uart_async_printf("kernel_main \t\t\t: kernal_main()\n");
        uart_async_printf("fork_test \t\t\t: fork_test();\n");
        uart_async_printf("execfile \t\t\t: execfile(syscall.img);\n");
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
        char *st = (char*)malloc(16);
        // uart_puts("test1\n");
        st[0] = 'a';
        st[1] = 'b';
        st[2] = 'c';
        st[3] = '\0';
        uart_printf("%s\n", st);
    }
    else if(!strcmp(arg[0], "exec") && i == 1) {
        execfile(arg[1]);
    }
    else if (!strcmp(arg[0], "sto") && i == 3) {
        // setTimeout MESSAGE SECONDS

        uart_async_printf("'%s' Set Timeout : %d\n", arg[1], atoi(arg[2]) * get_clock_freq() + get_clock_tick());
        add_timer(uart_puts, arg[1], atoi(arg[2]), 0);
    }
    else if (!strcmp(arg[0], "tsa") && i == 2) {
        // tsa  MESSAGE

        add_timer(two_second_alert, arg[1], 2, 0);
        
    }
    else if (!strcmp(arg[0], "async") && i == 1) {
        char c = ' ';
        uart_async_printf("Press enter to exit\n");
        while(c != '\n' && c != '\r') {
            c = uart_async_getc();
            uart_async_putc(c);
        }
        uart_async_printf("\n");

    }
    else if(!strcmp(arg[0], "preempt") && i == 1) {
        test_preemption();
    }
    else if(!strcmp(arg[0], "pfa") && i == 1) {
        page_frame_allocator_test();
    }
    else if(!strcmp(arg[0], "csa") && i == 1) {
        chunk_slot_allocator_test();
    }
    else if(!strcmp(arg[0], "kernel_main") && i == 1) {
        kernel_main();
    }
    else if(!strcmp(arg[0], "fork_test") && i == 1) {
        fork_test();
    }
    else if(!strcmp(arg[0], "execfile") && i == 1) {
        execfile(arg[1]);
    }
    else{ 
        uart_async_printf("Unknown command: %s\n", s1);
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
