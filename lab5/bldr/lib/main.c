#include "shell.h"
#include "relocate.h"

extern char *dtb_base;
int relocated = 1;

void main(char *arg)
{
    dtb_base = arg;

    if (relocated)
    {
        relocated = 0;
        relocate(arg);
    }

    uart_init();
    shell();
}