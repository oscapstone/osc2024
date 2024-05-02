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
extern void* _kernel_start;
int cmd_list_size = 0;

struct CLI_CMDS cmd_list[] =
{
    {.command="help",                   .func=do_cmd_help,          .help="print all available commands"},
    {.command="exec",                   .func=do_cmd_exec,          .help="execute a command, replacing current image with a new image"},
    {.command="setTimeout",             .func=do_cmd_setTimeout,    .help="setTimeout [MESSAGE] [SECONDS]"},
    {.command="memory_tester",          .func=do_cmd_memory_tester, .help="memory testcase generator, allocate and free"},
    {.command="cat",                    .func=do_cmd_cat,           .help="concatenate files and print on the standard output"},
    {.command="dtb",                    .func=do_cmd_dtb,           .help="show device tree"},
    {.command="info",                   .func=do_cmd_info,          .help="get device information via mailbox"},
    {.command="ls",                     .func=do_cmd_ls,            .help="list directory contents"},
    {.command="hello",                  .func=do_cmd_hello,         .help="print Hello World!"},
    {.command="reboot",                 .func=do_cmd_reboot,        .help="reboot the device"},
    {.command="c",                      .func=do_cmd_cancel_reboot, .help="cancel reboot the device"}
};
void cli_cmd_init()
{
    cmd_list_size = sizeof(cmd_list) / sizeof(struct CLI_CMDS);
}
void cli_cmd()
{
    cli_print_banner();
    char input_buffer[CMD_MAX_LEN];
    while(1){
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts("# ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
}
void cli_cmd_clear(char* buffer, int length)
{
    for(int i=0; i<length; i++)
    {
        buffer[i] = '\0';
    }
};

void cli_cmd_read(char* buffer)
{
    char c = '\0';
    int idx = 0;
    while(1)
    {
        if ( idx >= CMD_MAX_LEN ) break;
        c = uart_async_getc();

        // if user key 'enter'
        if ( c == '\n')
        {
            uart_puts("\r\n");
            buffer[idx] = '\0';
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
            for(int tab_index = 0; tab_index < cmd_list_size; tab_index++)
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
                    idx = strlen(buffer);
                    break;
                }
            }
            continue;
        }

        // some ascii blacklist
        if ( c > 16 && c < 32 ) continue;
        if ( c > 127 ) continue;

        buffer[idx++] = c;
        // uart_send(c); // we don't need this anymore

    }
}

void cli_cmd_exec(char* buffer)
{
    if (!buffer) return;

    char *words[3] = {NULL, NULL, NULL};
    int argc = str_SepbySpace(buffer, words) - 1;

    char* cmd       = words[0];
    char* argvs[2]  = {words[1], words[2]};
    // argvs[0] = words[1];
    // argvs[1] = words[2];

    for (int i = 0; i < cmd_list_size; i++)
    {
        if (strcmp(cmd, cmd_list[i].command) == 0)
        {
            cmd_list[i].func(argvs, argc);
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
    uart_puts("  OSC 2024 Lab6 Shell  \r\n");
    uart_puts("=======================================\r\n");
}

DO_CMD_FUNC(do_cmd_cat)
{
    char* filepath = argv[0];
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
    return 0;
}

DO_CMD_FUNC(do_cmd_dtb)
{
    traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
    return 0;

}
DO_CMD_FUNC(do_cmd_memory_tester)
{

    char *p1 = kmalloc(0x820);
    char *p2 = kmalloc(0x900);
    char *p3 = kmalloc(0x2000);
    char *p4 = kmalloc(0x3900);
    kfree(p3);
    kfree(p4);
    kfree(p1);
    kfree(p2);

    // char *p[10];
    // for (int i = 0; i < 10; i++)
    // {
    //     p[i] = kmalloc(0x1000);
    // }

    // for (int i = 0; i < 10;i++)
    // {
    //     kfree(p[i]);
    // }

    char *a = kmalloc(0x10);        // 16 byte
    char *b = kmalloc(0x100);
    char *c = kmalloc(0x1000);

    kfree(a);
    kfree(b);
    kfree(c);

    a = kmalloc(32);
    char *aa = kmalloc(50);
    b = kmalloc(64);
    char *bb = kmalloc(64);
    c = kmalloc(128);
    char *cc = kmalloc(129);
    char *d = kmalloc(256);
    char *dd = kmalloc(256);
    char *e = kmalloc(512);
    char *ee = kmalloc(999);

    char *f = kmalloc(0x2000);
    char *ff = kmalloc(0x2000);
    char *g = kmalloc(0x2000);
    char *gg = kmalloc(0x2000);
    char *h = kmalloc(0x2000);
    char *hh = kmalloc(0x2000);

    kfree(a);
    kfree(aa);
    kfree(b);
    kfree(bb);
    kfree(c);
    kfree(cc);
    kfree(dd);
    kfree(d);
    kfree(e);
    kfree(ee);

    kfree(f);
    kfree(ff);
    kfree(g);
    kfree(gg);
    kfree(h);
    kfree(hh);

    return 0;
}
DO_CMD_FUNC(do_cmd_help)
{
    for(int i = 0; i < CLI_MAX_CMD; i++)
    {
        uart_puts(cmd_list[i].command);
        uart_puts("\t\t\t: ");
        uart_puts(cmd_list[i].help);
        uart_puts("\r\n");
    }
    return 0;

}

DO_CMD_FUNC(do_cmd_exec)
{
    char* filepath = argv[0];
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

    filepath = "vm.img";
    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if(error) break;

        if(strcmp(c_filepath, filepath)==0)
        {
            uart_recv_echo_flag = 0; // syscall.img has different mechanism on uart I/O.
            thread_exec(c_filedata, c_filesize);
            break;
        }
        if(header_ptr==0) uart_puts("cat: %s: No such file or directory\n", filepath);
    }
    return 0;
}

DO_CMD_FUNC(do_cmd_hello)
{
    uart_puts("Hello World!\r\n");
    return 0;
}
DO_CMD_FUNC(do_cmd_info)
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
    return 0;
}

DO_CMD_FUNC(do_cmd_ls)
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
    return 0;
}

DO_CMD_FUNC(do_cmd_setTimeout)
{
    char* msg = argv[0];
    char* sec = argv[1];

    if (msg == NULL || sec == NULL)
    {
        uart_puts("Usage: setTimeout [MESSAGE] [SECONDS]\r\n");
        return 0;
    }
    add_timer(uart_sendline,atoi(sec),msg,0);
    return 0;
}

DO_CMD_FUNC(do_cmd_reboot)
{
    // uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    char* kernel_start = (char*) (&_kernel_start);
    uart_sendline("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;

    unsigned long long expired_tick = 10 * 10000;

    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = (unsigned long long)PM_PASSWORD | expired_tick;
    ((void (*)(char*))kernel_start)(dtb_ptr);
    return 0;
}

DO_CMD_FUNC(do_cmd_cancel_reboot)
{
    uart_puts("Cancel Reboot \r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x0;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 0;
    return 0;
}

