#include "headers/uart.h"
#include "headers/utils.h"

void get_actual_img()
{
    char *kernel = (char*)(0x80000); 
    int ind = 0;

    char size_str[50];
    char c;
    while(1)
    {
        c = receive();
        send(c);
        send('\n');
        if(!((c>='0') && (c<='9')))
        {
            size_str[ind] = '\0';
            break;
        }
        size_str[ind++] = c;
    }

    unsigned size = atoi_dec(size_str, strlen(size_str));

    ind = 0;
    while(size--)
    {
        kernel[ind++] = receive();
        display("Receive one byte\n");
    }
    display("Finish\n");

    asm volatile(
       "mov x0, x10;"
       "mov x30, 0x80000;"
       "ret;"
    );
}

int bootloader_main()
{
    init();
    display("First stage bootloader...\n");
    get_actual_img();

    return 0;
}