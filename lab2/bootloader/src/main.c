#include "uart1.h"
#include "shell.h"


char *_dtb = 0;

/* x0-x7 are argument registers.
   x0 is now used for dtb */
void main(char *arg)
{
    _dtb = arg;
    uart_init();
    start_shell(_dtb);
}
