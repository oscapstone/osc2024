#include "shell.h"
#include "string.h"
#include "command.h"
#include "uart.h"
#include "tasklist.h"
#include "heap.h"
void loop()
{
    /*unsigned int sp;
    asm volatile("mov %0, sp":"=r"(sp):);
    uart_hex(sp);*/
    uart_puts("=> ");
    while (1)
    {
        execute_tasks();
    }
}

void shell_start()
{
    int buffer_counter = 0;
    char input_char;
    char buffer[MAX_BUFFER_LEN];
    enum SPECIAL_CHARACTER input_parse;

    strset(buffer, 0, MAX_BUFFER_LEN);

    // new line head
    //uart_puts("=> ");

    // read input
    while (1)
    {
        input_char = uart_async_read();
        input_parse = parse(input_char);

        command_controller(input_parse, input_char, buffer, &buffer_counter);
        if(input_parse == NEW_LINE)
            break;
    }
}

enum SPECIAL_CHARACTER parse(char c)
{
    // uart_send(c);
    if (!(c < 128 && c > 0))
        return UNKNOWN;

    if (c == BACK_SPACE)
    {
        return BACK_SPACE;
    }
    else if (c == LINE_FEED || c == CARRIAGE_RETURN)
        return NEW_LINE;
    else
        return REGULAR_INPUT;
}

void command_controller(enum SPECIAL_CHARACTER input_parse, char c, char buffer[], int *counter)
{
    if (input_parse == UNKNOWN)
        return;

    if (input_parse == BACK_SPACE)
    {
        if ((*counter) > 0)
        {
            (*counter)--;
            uart_puts("\b \b");
        }
    }
    else if (input_parse == NEW_LINE)
    {
        uart_send(c);
        char *input_string = buffer;
        char *parameter[5]; // 5 is the available parameter length
        int para_idx = 0;
        int input_string_len = utils_strlen(input_string);
        for (int i = 0; i < input_string_len; i++)
        {
            if (*(input_string + i) == ' ')
            {
                *(input_string + i) = '\0';
                parameter[para_idx++] = (input_string + i + 1);
            }
        }

        if ((*counter) == MAX_BUFFER_LEN)
        {
            input_buffer_overflow_message(buffer);
        }
        else
        {
            buffer[(*counter)] = '\0';
            //uart_send('\n');

            if (!strcmp(input_string, "help"))
                command_help();
            else if (!strcmp(input_string, "hello"))
                command_hello();
            else if (!strcmp(input_string, "reboot"))
                command_reboot();
            else if (!strcmp(input_string, "dtb"))
                command_dtb();
            else if (!strcmp(input_string, "el"))
                command_show_el();
            else if (!strcmp(input_string, "ls"))
                command_ls();
            else if (!strcmp(input_string, "cat"))
                command_cat(parameter[0]);
            else if (!strcmp(input_string, "run"))
                command_run();
            else if (!strcmp(input_string, "timer_on"))
                command_timer_on();
            else if (!strcmp(input_string, "timer_off"))
                command_timer_off();
            else if (!strcmp(input_string, "set_timeout"))
                command_set_timeout(parameter[0], parameter[1]);
            else
                command_not_found(input_string);
        }

        (*counter) = 0;
        strset(buffer, 0, MAX_BUFFER_LEN);

        // new line head;
        uart_puts("=> ");
    }
    else if (input_parse == REGULAR_INPUT)
    {
        // uart_puts("::");
        // uart_send(c);

        if (*counter < MAX_BUFFER_LEN)
        {
            buffer[*counter] = c;
            (*counter)++;
        }
    }
}