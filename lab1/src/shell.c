#include "mini_uart.h"
#include "command.h"

#define MAX_STR_BUFFER 128u

int strcmp(char *str1, char *str2)
{
    char *ptr1 = str1;
    char *ptr2 = str2;
    while(1)
    {
        char c1 = *ptr1;
        ptr1++;
        char c2 = *ptr2;
        ptr2++;

        if(c1 == c2)
        {
            if(c1 == '\0') return 0;
            else continue;
        }
        else 
        {
            return 1;
        }
    }
}

int read_cmd(char *str)
{
    int cur_save = 0, len = 0, start_flag = 0, final_cmd_next = 0, overflow_flag = 0;
    char c;

    str[0] = '\0';
    while((c = uart_recv()) != '\r')
    {
        if(len > MAX_STR_BUFFER)
        {
            overflow_flag = 1;
            continue;
        }

        if(c == 127)              // if ascii = backspace
        {
            if(cur_save > 0)
            {
                cur_save--;
                for(int i = cur_save; i < len; i++){
                    str[i] = str[i+1];
                }
                str[--len] = '\0';
                uart_send_string("\b \b");
            }
        }
        else if(c == 32 && start_flag == 0) // if ascii = space in front of cmd
        {
            continue;
        }
        else
        {   
            start_flag = 1;
            if(cur_save < len)
            {
                for(int i = len; i > cur_save; i--)
                {
                    str[i] = str[i-1];
                }
            }
            str[cur_save++] = c;
            
            str[++len] = '\0';
            if (c != 32)
            {
                final_cmd_next = len;
            }
            
        }
        uart_send(c);
    }
    uart_send_string("\r\n");

    str[final_cmd_next] = '\0';
    if (overflow_flag == 1)
        return 1;
    else
        return 0;
}

void parse_cmd(char* str)
{
    if(!strcmp(str,"")){
        return;
    }
    else if(!strcmp(str,"help"))
    {
        cmd_help();
    }
    else if(!strcmp(str,"hello"))
    {
        cmd_hello();
    }
    else if(!strcmp(str,"reboot"))
    {
        cmd_reboot();
    }
    else if(!strcmp(str,"cancel reboot"))
    {
        cmd_cancel_reboot();
    }
    else if(!strcmp(str,"mailbox"))
    {
        cmd_mailbox();
    }
    else
    {
        cmd_not_found();
    }
}

void shell()
{   
	uart_send_string("Hello, OSC 2024 Lab1!\r\n");
    while(1)
    {
        char cmd[MAX_STR_BUFFER];
        uart_send_string("# ");
        int ret = read_cmd(cmd);
        if(ret == 0)
        {
            parse_cmd(cmd);
        }
        else
        {uart_send_string("here\r\n");
            uart_send_string("Shell: input command too long.\n");
        }
    }
}
