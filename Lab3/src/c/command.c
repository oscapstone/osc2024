#include "uart.h"
#include "string.h"
#include "cpio.h"
#include "dtb.h"
#include "tasklist.h"
#include "timer.h"


extern void *_dtb_ptr;
int user_timer = 0;

void input_buffer_overflow_message(char cmd[])
{
    uart_puts("Follow command: \"");
    uart_puts(cmd);
    uart_puts("\"... is too long to process.\n");

    uart_puts("The maximum length of input is 64.");
}

void command_help()
{
    uart_puts("\n");
    uart_puts("Valid Command:\n");
    uart_puts("\rhelp: print this help.\n");
    uart_puts("\rhello: print \"Hello World!\".\n");
    uart_puts("\rreboot: reboot Raspberry Pi.\n");
    uart_puts("\rdtb: print all device.\n");
    uart_puts("\rel: show exception level.\n");
    uart_puts("\rls: read initramfs.cpio on the SD card.\n");
    uart_puts("\rcat <filename>: Search for a specific file.\n");
    uart_puts("\rrun: Run an executable.\n");
    uart_puts("\rtimer_on: turn on the core timer.\n");
    uart_puts("\rtimer_off: turn off the core timer.\n");
    uart_puts("\rset_timeout <second> <message>: set a user timeout.\n");
    uart_puts("\n");
}

void command_hello()
{
    uart_puts("Hello World!\n");
}

void command_dtb(){
    fdt_traverse(print_dtb,_dtb_ptr);
}

void command_show_el(){
    uart_puts("Current EL is:");
    unsigned long el;
    asm volatile("mrs %0, CurrentEL" : "=r"(el));

    uart_dec((char)(el >> 2)&3);
    uart_send('\n');
}
void command_ls()
{
    cpio_ls();
}



void command_cat(char *filename)
{   
    // disable_irq();
    // char file_name[100];
    // uart_puts("file name: ");
    // uart_getline(file_name);
    //uart_puts(file_name);
    //enable_irq();
    cpio_cat(filename);
}

void command_run()
{
    //char file_name[100];
    
    //printf("file name: ");
    //uart_getline(file_name);

    cpio_run_executable("syscall.img");
}

void command_timer_on()
{
    //asm volatile("svc 1");
    uart_puts("enable core timer\n");
    core_timer_enable();
}

void command_timer_off()
{
    uart_puts("disable core timer\n");
    core_timer_disable();
}

void command_set_timeout(char *second_string, char *message)
{
    //disable_irq();
    user_timer = 1;
    /*char second_string[10];
    uart_puts("time: ");
    uart_getline(second_string);*/
    int second = atoi(second_string);
    //uart_puts("\n");

    //char message[100];
    // uart_puts("message: ");
    // uart_getline(message);
    // uart_puts("\n");
    // enable_irq();

    asm volatile(
        "mov x10, %0    \n\t"
        "mov x11, %1    \n\t"
        :
        : "r"(&second), "r"(message));

    set_new_timeout();
}

void command_not_found(char *s)
{
    uart_puts("Err: command ");
    uart_puts(s);
    uart_puts(" not found, try <help>\n");
}

void command_reboot()
{
    uart_puts("Start Rebooting...\n");

    *PM_WDOG = PM_PASSWORD | 100;
    *PM_RSTC = PM_PASSWORD | 0x20;

    // while(1);
}