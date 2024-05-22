#include "shell.h"

void shell()
{
    char cmd[MAX_BUF_SIZE];

    while (1)
    {
        uart_printf("# ");
        uart_gets(cmd);
        cmd_resolve(cmd);
    }
}
void cmd_resolve(char *cmd)
{
    char argv[100][MAX_BUF_SIZE];
    unsigned int argc = 0;
    unsigned int cmd_length = strlen(cmd);
    int i, j;

    /* String processing */
    for (i = 0, j = 0; i < cmd_length; ++i)
    {
        if (cmd[i] == ' ')
        {
            argv[argc][j] = '\0';
            j = 0;
            continue;
        }
        argv[argc][j++] = cmd[i];
    }
    argv[argc++][j] = '\0';

    if (!strcmp(cmd, "help"))
    {
        uart_printf("help                            : print this help menu\n");
        uart_printf("boot                            : boot the kernel, please provide the filename\n");
        uart_printf("clear                           : clear page\n");
    }
    else if (!strcmp(cmd, "boot"))
        loadIMG();
    else if (!strcmp(cmd, "clear"))
        uart_printf("\x1b[H\x1b[J");
    else
        uart_printf("Unknown command: %s\n", argv[0]);
}

