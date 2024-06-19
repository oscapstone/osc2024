#include "mini_uart.h"
#include "command.h"
#include "c_utils.h"

#define MAX_STR_BUFFER 128u

int read_cmd(char *str)
{
    int cur_save = 0, len = 0, start_flag = 0, final_cmd_next = 0, overflow_flag = 0;
    char c;

    str[0] = '\0';
    while((c = uart_async_recv()) != '\r')
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
                uart_async_send_string("\b \b");
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
    uart_async_send_string("\r\n");

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
    else if(!strcmp(str,"ls"))
    {
        cmd_ls();
    }
    else if(!strcmp(str,"cat"))
    {
        uart_async_send_string("Enter file name: ");
        char filename[MAX_STR_BUFFER];
        read_cmd(filename);
        cmd_cat(filename);
    }
    else if(!strcmp(str,"malloc"))
    {
        uart_async_send_string("Enter malloc size: ");
        char m_size[MAX_STR_BUFFER];
        read_cmd(m_size);
        int m_size_i = atoi(m_size);
        cmd_malloc(m_size_i);
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
    else if(!strcmp(str,"exec"))
    {
        uart_async_send_string("Enter file name: ");
        char filename[MAX_STR_BUFFER];
        read_cmd(filename);
        cmd_exec(filename);
    } 
    else if (!strncmp(str, "simpleTimer", 11))
    {
        char seconds[256];
        strncpy(seconds, str + 12, strlen(str) - 11);
        unsigned int timeout = atoi(seconds);
        cmd_simple_timer(timeout, seconds);
    } 
    else if (!strncmp(str, "setTimeout", 10))
    {        
        int j = 11;
        while(str[j] != ' ')
        {
            if(j >= 256)
            {
                uart_async_send_string("Error: setTimeout format.\r\n");
                return;
            }
            j++;
        }

        char message[256];
        strncpy(message, str + 11, j - 11);
        message[j - 11] = '\0';

        char seconds[256];
        strncpy(seconds, str + j + 1, strlen(str) - j);
        
        unsigned int timeout = atoi(seconds);
        cmd_timeout(timeout, message, seconds);
    } 
    else
    {
        cmd_not_found();
    }
}

void shell()
{   
    uart_async_send_string("\r\n\r\n");
	uart_async_send_string("##########################\r\n");
    uart_async_send_string("#  Hello, OSC 2024 Lab!  #\r\n");
    uart_async_send_string("##########################\r\n");
    
    while(1)
    {
        char cmd[MAX_STR_BUFFER];
        uart_async_send_string("# ");
        int ret = read_cmd(cmd);
        if(ret == 0)
        {
            parse_cmd(cmd);
        }
        else
        {
            uart_async_send_string("Shell: input command too long.\n");
        }
    }
}
