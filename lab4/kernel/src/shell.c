#include <stddef.h>
#include "shell.h"
#include "uart1.h"
#include "mbox.h"
#include "power.h"
#include "cpio.h"
#include "u_string.h"
#include "dtb.h"
#include "memory.h"
#include "timer.h"

#define USTACK_SIZE 0x10000

extern char* dtb_ptr;
extern void* CPIO_DEFAULT_START;

int cmd_list_size = 0;
struct CLI_CMDS cmd_list[] =
{
    {.command="help",                   .func=do_cmd_help,          .help="print all available commands"},
    {.command="memory_tester",          .func=do_cmd_memory_tester, .help="memory testcase generator, allocate and free"},
    {.command="s_allocator",            .func=do_cmd_s_allocator,   .help="simple allocator in heap session"},
};
void cli_cmd_init()
{
    cmd_list_size = sizeof(cmd_list) / sizeof(struct CLI_CMDS);
}
void cli_cmd()
{
    char input_buffer[CMD_MAX_LEN];
    while(1){
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts("# ");
        // uart_sendline("# ");
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
    uart_puts("  OSC 2024 Lab4 Shell  \r\n");
    uart_puts("=======================================\r\n");
}

DO_CMD_FUNC(do_cmd_help)
{
    for(int i = 0; i < cmd_list_size; i++)
    {
        uart_puts(cmd_list[i].command);
        uart_puts("\t\t\t: ");
        uart_puts(cmd_list[i].help);
        uart_puts("\r\n");
    }
    return 0;
}


DO_CMD_FUNC(do_cmd_s_allocator)
{
    //test malloc
    char* test1 = init_malloc(0x18);
    memcpy(test1,"test malloc1",sizeof("test malloc1"));
    uart_puts("%s\n",test1);

    char* test2 = init_malloc(0x20);
    memcpy(test2,"test malloc2",sizeof("test malloc2"));
    uart_puts("%s\n",test2);

    char* test3 = init_malloc(0x28);
    memcpy(test3,"test malloc3",sizeof("test malloc3"));
    uart_puts("%s\n",test3);

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