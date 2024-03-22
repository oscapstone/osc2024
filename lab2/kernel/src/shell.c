#include <stddef.h>
#include "shell.h"
#include "uart1.h"
#include "mbox.h"
#include "power.h"
#include "cpio.h"
#include "u_string.h"
#include "dtb.h"
#include "heap.h"

#define CLI_MAX_CMD 9

extern char* dtb_ptr; // it's the address of dtb and it declared in dtb.c
void* CPIO_DEFAULT_PLACE; // it's the address of cpio

struct CLI_CMDS cmd_list[CLI_MAX_CMD]=
{
    {.command="cat",    .func=do_cmd_cat,           .help="concatenate files and print on the standard output"},
    {.command="dtb",    .func=do_cmd_dtb,           .help="show device tree"},
    {.command="hello",  .func=do_cmd_hello,         .help="print Hello World!"},
    {.command="help",   .func=do_cmd_help,          .help="print all available commands"},
    {.command="info",   .func=do_cmd_info,          .help="get device information via mailbox"},
    {.command="malloc", .func=do_cmd_malloc,        .help="simple allocator in heap session"},
    {.command="ls",     .func=do_cmd_ls,            .help="list directory contents"},
    {.command="reboot", .func=do_cmd_reboot,        .help="reboot the device"},
    {.command="c",      .func=do_cmd_cancel_reboot, .help="cancel reboot the device"}
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

        c = uart_recv();

        // if user key 'enter'
        if ( c == '\n')
        {
            uart_puts("\r\n");
            break;
        }

        // if user key 'backspace'
        if ( c == '\b' || c == 127 )
        {
            if ( idx > 0 )
            {
                uart_puts("\b \b");
                idx--;
                buffer[idx] = '\0';
            }
            continue;
        }

        // use tab to auto complete
        if ( c == '\t' )
        {
            for(int tab_index = 0; tab_index < CLI_MAX_CMD; tab_index++)
            {
                if (strncmp(buffer, cmd_list[tab_index].command, strlen(buffer)) == 0)
                {
                    for (int j = 0; j < strlen(buffer); j++)
                    {
                        uart_puts("\b \b");
                    }
                    uart_puts(cmd_list[tab_index].command);
                    cli_cmd_clear(buffer, strlen(buffer) + 3);
                    strcpy(buffer, cmd_list[tab_index].command);
                    break;
                }
            }
            continue;
        }

        // some ascii blacklist
        if ( c > 16 && c < 32 ) continue;
        if ( c > 127 ) continue;

        buffer[idx++] = c;
        uart_send(c);
    }
}

void cli_cmd_exec(char* buffer)
{
    if (!buffer) return;

    char* cmd = buffer;
    char* argvs;

    while(1){
        if(*buffer == '\0')
        {
            argvs = buffer;
            break;
        }
        if(*buffer == ' ')
        {
            *buffer = '\0';
            argvs = buffer + 1;
            break;
        }
        buffer++;
    }
    for (int i = 0; i < CLI_MAX_CMD; i++)
    {
        if (strcmp(cmd, cmd_list[i].command) == 0)
        {
            cmd_list[i].func(argvs, 1);
            return;            
        }
    }
    if (*cmd != '\0')
    {
        uart_puts(cmd);
        uart_puts(": command not found\r\n");
    }
}

void cli_print_banner()
{
    uart_puts("\r\n");
    uart_puts("=======================================\r\n");
    uart_puts("  OSC 2024 Shell Lab2 - Kernel        \r\n");
    uart_puts("=======================================\r\n");
}

int do_cmd_cat(char *argv, int argc)
{
    char* filepath = argv;
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        //if parse header error
        if(error)
        {
            uart_puts("cpio parse error");
            break;
        }

        if(strcmp(c_filepath, filepath)==0)
        {
            uart_puts("%s", c_filedata);
            break;
        }

        //if this is TRAILER!!! (last of file)
        if(header_ptr==0) uart_puts("cat: %s: No such file or directory\n", filepath);
    }
    return 0;
}

int do_cmd_dtb(char *argv, int argc)
{
    traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
    return 0;
}

int do_cmd_help(char *argv, int argc)
{
    for(int i = 0; i < CLI_MAX_CMD; i++)
    {
        uart_puts(cmd_list[i].command);
        uart_puts("\t\t: ");
        uart_puts(cmd_list[i].help);
        uart_puts("\r\n");
    }
    return 0;
}

int do_cmd_hello(char *argv, int argc)
{
    uart_puts("Hello World!\r\n");
    return 0;
}

int do_cmd_info(char *argv, int argc)
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
        uart_puts("Hardware Revision\t: 0x%x\r\n", pt[5]);
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
        uart_puts("ARM Memory Base Address\t: 0x%x\r\n", pt[5]);
        uart_puts("ARM Memory Size\t\t: %d bytes\r\n", pt[6]);
    }
    return 0;
}

int do_cmd_malloc(char *argv, int argc)
{
    //test malloc
    char* test1 = malloc(0x18);
    memcpy(test1,"test malloc1",sizeof("test malloc1"));
    uart_puts("%s: address: 0x%x, size: %ld bytes\n",test1, test1, *(test1-0x8));

    char* test2 = malloc(0x20);
    memcpy(test2,"test malloc2",sizeof("test malloc2"));
    uart_puts("%s: address: 0x%x, size: %ld bytes\n",test2, test2, *(test2-0x8));

    char* test3 = malloc(0x28);
    memcpy(test3,"test malloc3",sizeof("test malloc3"));
    uart_puts("%s: address: 0x%x, size: %ld bytes\n",test3, test3, *(test3-0x8));
    return 0;
}

int do_cmd_ls(char *argv, int argc)
{
    // char* workdir = argv;
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        //if parse header error
        if(error)
        {
            uart_puts("cpio parse error");
            break;
        }

        //if this is not TRAILER!!! (last of file)
        if(header_ptr!=0) {
            uart_puts("%s\n", c_filepath);
        }
    }
    return 0;
}


int do_cmd_reboot(char *argv, int argc)
{
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;

    unsigned long long expired_tick = 10 * 10000;

    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = (unsigned long long)PM_PASSWORD | expired_tick;
    return 0;
}

int do_cmd_cancel_reboot(char *argv, int argc)
{
    uart_puts("Cancel Reboot \r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x0;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 0;
    return 0;
}

