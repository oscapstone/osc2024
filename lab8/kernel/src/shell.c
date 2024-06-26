#include <stddef.h>
#include "shell.h"
#include "uart1.h"
#include "mbox.h"
#include "power.h"
#include "cpio.h"
#include "string.h"
#include "dtb.h"
#include "memory.h"
#include "timer.h"
#include "sched.h"

#define CLI_MAX_CMD 9

extern int   uart_recv_echo_flag;
extern char* dtb_ptr;
extern void* CPIO_DEFAULT_START;

struct CLI_CMDS cmd_list[CLI_MAX_CMD]=
{
    {.command="cat", .help="concatenate files and print on the standard output"},
    {.command="dtb", .help="show device tree"},
    {.command="exec", .help="execute a command, replacing current image with a new image"},
    {.command="hello", .help="print Hello World!"},
    {.command="help", .help="print all available commands"},
    {.command="info", .help="get device information via mailbox"},
    {.command="ls", .help="list directory contents"},
    {.command="setTimeout", .help="setTimeout [MESSAGE] [SECONDS]"},
    {.command="reboot", .help="reboot the device"}
};

void cli_cmd_clear(char* buffer, int length)
{
    for(int i=0; i<length; i++)
    {
        buffer[i] = '\0';
    }
};

void cli_cmd_read(char* buffer)
{
    char c='\0';
    int idx = 0;
    while(1)
    {
        if ( idx >= CMD_MAX_LEN ) break;
        c = uart_async_getc();
        if ( c == '\n') break;
        buffer[idx++] = c;
    }
}

void cli_cmd_exec(char* buffer)
{
    if (!buffer) return;

    char* cmd = buffer;
    char* argvs = str_SepbySpace(buffer);

    if (strcmp(cmd, "cat") == 0) {
        do_cmd_cat(argvs);
    } else if (strcmp(cmd, "dtb") == 0){
        do_cmd_dtb();
    } else if (strcmp(cmd, "exec") == 0){
        do_cmd_exec(argvs);
    } else if (strcmp(cmd, "hello") == 0) {
        do_cmd_hello();
    } else if (strcmp(cmd, "help") == 0) {
        do_cmd_help();
    } else if (strcmp(cmd, "info") == 0) {
        do_cmd_info();
    } else if (strcmp(cmd, "ls") == 0) {
        do_cmd_ls(argvs);
    } else if (strcmp(cmd, "setTimeout") == 0) {
        char* sec = str_SepbySpace(argvs);
        do_cmd_setTimeout(argvs, sec);
    } else if (strcmp(cmd, "reboot") == 0) {
        do_cmd_reboot();
    }
}

void cli_print_banner()
{
    //uart_puts("\r\n");
    //uart_puts("=======================================\r\n");
    //uart_puts("  Welcome to NYCU-OSC 2023 Lab8 Shell  \r\n");
    //uart_puts("=======================================\r\n");
    uart_puts("Lab8:\r\n");
}

void do_cmd_cat(char* filepath)
{
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if(error) break;
        if(strcmp(c_filepath, filepath)==0)
        {
            uart_puts("%s", c_filedata);
            break;
        }
        if(header_ptr==0) uart_puts("cat: %s: No such file or directory\n", filepath);
    }
}

void do_cmd_dtb()
{
    traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
}

void do_cmd_help()
{
    for(int i = 0; i < CLI_MAX_CMD; i++)
    {
        uart_puts(cmd_list[i].command);
        uart_puts("\t\t\t: ");
        uart_puts(cmd_list[i].help);
        uart_puts("\r\n");
    }
}

void do_cmd_exec(char* filepath)
{
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if(error) break;

        if(strcmp(c_filepath, filepath)==0)
        {
            uart_recv_echo_flag = 0; // syscall.img has different mechanism on uart I/O.
            thread_exec(c_filedata, c_filesize);
        }
        if(header_ptr==0) uart_puts("cat: %s: No such file or directory\n", filepath);
    }

}

void do_cmd_hello()
{
    uart_puts("Hello World!\r\n");
}

void do_cmd_info()
{
    // print hw revision
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_BOARD_REVISION;
    pt[3] = 4;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)) ) {
        uart_puts("Hardware Revision\t: ");
        uart_2hex(pt[6]);
        uart_2hex(pt[5]);
        uart_puts("\r\n");
    }
    // print arm memory
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_ARM_MEMORY;
    pt[3] = 8;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)) ) {
        uart_puts("ARM Memory Base Address\t: ");
        uart_2hex(pt[5]);
        uart_puts("\r\n");
        uart_puts("ARM Memory Size\t\t: ");
        uart_2hex(pt[6]);
        uart_puts("\r\n");
    }
}

void do_cmd_ls(char* workdir)
{
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if(error) break;
        if(header_ptr!=0) uart_puts("%s\n", c_filepath);
    }
}

void do_cmd_setTimeout(char* msg, char* sec)
{
    add_timer(uart_sendline,atoi(sec),msg,0);
}

void do_cmd_reboot()
{
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}

