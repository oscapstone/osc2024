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

extern char *dtb_ptr;
extern void *CPIO_START;
extern int uart_recv_echo_flag;

void cmd_clear(char *buffer, int len)
{
    for (int i = 0; i < len; i++)
    {
        buffer[i] = '\0';
    }
}

void cmd_read(char *buffer)
{
    char c = '\0';
    int idx = 0;
    while (1)
    {
        if (idx >= CMD_MAX_LEN)
            break;
        c = uart_async_getc();
        if (c == '\n')
            break;
        buffer[idx++] = c;
    }
}

void cmd_exec(char *buffer)
{
    if (!buffer)
        return;

    char *cmd = buffer;
    char *argvs = str_arg(buffer);

    if (strcmp(cmd, "hello") == 0)
    {
        cmd_hello();
    }
    else if (strcmp(cmd, "info") == 0)
    {
        cmd_info();
    }
    else if (strcmp(cmd, "reboot") == 0)
    {
        cmd_reboot();
    }
    else if (strcmp(cmd, "help") == 0)
    {
        cmd_help();
    }
    else if (strcmp(cmd, "dtb") == 0)
    {
        cmd_dtb();
    }
    else if (strcmp(cmd, "ls") == 0)
    {
        cmd_ls();
    }
    else if (strcmp(cmd, "cat") == 0)
    {
        cmd_cat(argvs);
    }
    else if (strcmp(cmd, "settimeout") == 0)
    {
        char *message = str_arg(argvs);
        cmd_timeout(argvs, message);
    }
    else if (strcmp(cmd, "run") == 0)
    {
        cmd_user_program(argvs);
    }
    else if (strcmp(cmd, "el") == 0)
    {
        cmd_el();
    }
    else
    {
        cmd_error(cmd);
    }
}

void cmd_hello()
{
    uart_sendline("Hello World!\n");
}

void cmd_info()
{
    // HW revision
    m[0] = 8 * 4;
    m[1] = MBOX_REQUEST_PROCESS;
    m[2] = MBOX_TAG_GET_BOARD_REVISION;
    m[3] = 4;
    m[4] = MBOX_TAG_REQUEST_CODE;
    m[5] = 0;
    m[6] = 0;
    m[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&m)))
    {
        uart_sendline("Hardware Revision: ");
        uart_2hex(m[6]);
        uart_2hex(m[5]);
        uart_sendline("\n");
    }

    // arm mem
    m[0] = 8 * 4;
    m[1] = MBOX_REQUEST_PROCESS;
    m[2] = MBOX_TAG_GET_ARM_MEMORY;
    m[3] = 4;
    m[4] = MBOX_TAG_REQUEST_CODE;
    m[5] = 0;
    m[6] = 0;
    m[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&m)))
    {
        uart_sendline("ARM Memory Base Address: ");
        uart_2hex(m[5]);
        uart_sendline("\n");
        uart_sendline("ARM Memory Size: ");
        uart_2hex(m[6]);
        uart_sendline("\n");
    }
}

void cmd_help()
{
    uart_sendline("help list:\n");
    uart_sendline("hello:\t\t\tprint Hello World\n");
    uart_sendline("help:\t\t\tprint all available command\n");
    uart_sendline("info:\t\t\tprint information of the board\n");
    uart_sendline("reboot:\t\t\treboot in 5 seconds\n");
    uart_sendline("dtb:\t\t\tprint the whole dtb\n");
    uart_sendline("ls:\t\t\tlist all file and directory\n");
    uart_sendline("cat:\t\t\tcat [file name] print the file contet\n");
    uart_sendline("el:\t\t\tshow exception level.\n");
    //uart_sendline("set2stimeout:\t\tevery 2 seconds print current time\n");
    uart_sendline("settimeout:\t\tsettimeout [seconde] [message] set an alert with message\n");
    //uart_sendline("memory:\t\t\ttest memory\n");
    //uart_sendline("s_allocator:\t\tsimple allocator\n");
    uart_sendline("run:\t\trun [image name] execute user program\n");
    //uart_sendline("thread:\t\ttest thread create by foo\n");
}

void cmd_dtb()
{
    traverse_device_tree(dtb_ptr, dtb_cb_print);
}

void cmd_ls()
{
    char *filepath;
    char *filecontent;
    unsigned int filesize;
    struct cpio_newc_header *head_ptr = CPIO_START;

    while (head_ptr != 0)
    {
        if (cpio_newc_parse(head_ptr, &filepath, &filesize, &filecontent, &head_ptr))
        {
            uart_sendline("cpio parse error");
            break;
        }

        if (head_ptr != 0)
        {
            uart_sendline("%s\n", filepath);
        }
    }
}

void cmd_cat(char *file)
{
    char *filepath;
    char *filecontent;
    unsigned int filesize;
    struct cpio_newc_header *head_ptr = CPIO_START;

    while (head_ptr != 0)
    {
        if (cpio_newc_parse(head_ptr, &filepath, &filesize, &filecontent, &head_ptr))
        {
            uart_sendline("cpio parse error");
            break;
        }

        if (strcmp(filepath, file) == 0)
        {
            uart_sendline("%s", filecontent);
            break;
        }

        if (head_ptr == 0)
        {
            uart_sendline("No such file or directory: %s\n", file);
        }
    }
}

// void cmd_2stimeout()
// {
//     add_timer(timer_2s, 2, "2stimer", 0);
// }

void cmd_timeout(char *sec, char *message)
{
    char res[20];
    strcpy(res, strcat("Timeout message: ", message));

    add_timer(uart_sendline, atoi(sec), res, 0);
}

void cmd_reboot()
{
    uart_sendline("Reboot in 5 seconds ...\n");
    volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}



void cmd_error(char *cmd)
{
    uart_sendline("No '");
    uart_sendline(cmd);
    uart_sendline("' command\n");
}

void cmd_user_program(char *file)
{
    char *filepath;
    char *filecontent;
    unsigned int filesize;
    struct cpio_newc_header *head_ptr = CPIO_START;

    while (head_ptr != 0)
    {
        if (cpio_newc_parse(head_ptr, &filepath, &filesize, &filecontent, &head_ptr))
        {
            uart_sendline("cpio parse error");
            break;
        }

        if (strcmp(filepath, file) == 0)
        {
            uart_recv_echo_flag = 0; // prevent print 2 times
            //give space to user program
            char *start = kmalloc(filesize);
            start = filecontent;
            exec_thread(start, filesize);
        }

        if (head_ptr == 0)
        {
            uart_sendline("No such file or directory: %s\n", file);
        }
    }
}

void cmd_el(){
    unsigned long el;
    asm volatile("mrs %0, CurrentEL" : "=r"(el));
    uart_sendline("Current EL is: %d\n", (el>>2)&3);
}
