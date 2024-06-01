#include <stddef.h>
#include "shell.h"
#include "uart1.h"
#include "mbox.h"
#include "power.h"
#include "cpio.h"
#include "utils.h"
#include "dtb.h"
#include "memory.h"
#include "timer.h"

#define CLI_MAX_CMD 12
#define USTACK_SIZE 0x10000

extern char* dtb_ptr;
extern void* CPIO_DEFAULT_START;

struct CLI_CMDS cmd_list[CLI_MAX_CMD]=
{
    {.command="hello", .help="print Hello World!"},
    {.command="help", .help="print all available commands"},
    {.command="info", .help="get device information via mailbox"},
    {.command="cat", .help="concatenate files and print on the standard output"},
    {.command="ls", .help="list directory contents"},
    {.command="malloc", .help="simple allocator in heap session"},
    {.command="dtb", .help="show device tree"},
    {.command="exec", .help="execute a command, replacing current image with a new image"},
    {.command="setTimeout", .help="setTimeout [MESSAGE] [SECONDS]"},
    {.command="set2sAlert", .help="set core timer interrupt every 2 second"},
    {.command="memTest", .help="memory testcase generator, allocate and free"},
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

        // c = uart_recv();
        c = uart_async_getc();
        if ( c == '\n')
        {
            // uart_puts("\r\n");
            break;
        }
        buffer[idx++] = c;
        // uart_send(c);
    }
}

void cli_cmd_exec(char* buffer)
{
    if (!buffer) return;

    char* cmd = buffer;
    char* argvs = str_SepbySpace(buffer);
    // char* argvs;

    // while(1){
    //     if(*buffer == '\0')
    //     {
    //         argvs = buffer;
    //         break;
    //     }
    //     if(*buffer == ' ')
    //     {
    //         *buffer = '\0';
    //         argvs = buffer + 1;
    //         break;
    //     }
    //     buffer++;
    // }

    if (strcmp(cmd, "hello") == 0) {
        do_cmd_hello();
    } else if (strcmp(cmd, "help") == 0) {
        do_cmd_help();
    } else if (strcmp(cmd, "info") == 0) {
        do_cmd_info();
    } else if (strcmp(cmd, "cat") == 0) {
        do_cmd_cat(argvs);
    } else if (strcmp(cmd, "ls") == 0) {
        do_cmd_ls(argvs);
    } else if (strcmp(cmd, "malloc") == 0) {
        do_cmd_malloc();
    } else if (strcmp(cmd, "dtb") == 0){
        do_cmd_dtb();
    } else if (strcmp(cmd, "exec") == 0){
        do_cmd_exec(argvs);
    } else if (strcmp(cmd, "memTest") == 0) {
        do_cmd_memory_tester();
    } else if (strcmp(cmd, "setTimeout") == 0) {
        char* sec = str_SepbySpace(argvs);
        do_cmd_setTimeout(argvs, sec);
    } else if (strcmp(cmd, "set2sAlert") == 0) {
        do_cmd_set2sAlert();
    } else if (strcmp(cmd, "reboot") == 0) {
        do_cmd_reboot();
    } else if (cmd){
        uart_puts(cmd);
        uart_puts(": command not found\r\n");
        uart_puts("Type 'help' to see command list.\r\n");
    }
}

void cli_print_banner()
{
    uart_puts("=======================================\r\n");
    uart_puts("       ------      ------     ------   \r\n");
    uart_puts("     //     //   //         //         \r\n");
    uart_puts("    //     //    ------    //          \r\n");
    uart_puts("   //     //          //  //           \r\n");
    uart_puts("    ------     ------      ------      \r\n");
    uart_puts("          2024 Lab4 Allocator          \r\n");
    uart_puts("=======================================\r\n");
}

void do_cmd_help()
{
    for(int i = 0; i < CLI_MAX_CMD; i++)
    {
        uart_puts(cmd_list[i].command);
        uart_puts("\t\t: ");
        uart_puts(cmd_list[i].help);
        uart_puts("\r\n");
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

void do_cmd_cat(char* filepath)
{
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_START;

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
        //if parse header error
        if(error)
        {
            uart_puts("cpio parse error");
            break;
        }

        //if this is not TRAILER!!! (last of file)
        if(header_ptr!=0) uart_puts("%s\n", c_filepath);
    }
}

void do_cmd_malloc()
{
    //test malloc
    char* test1 = malloc(0x18);
    memcpy(test1,"test malloc1",sizeof("test malloc1"));
    uart_puts("%s\n",test1);

    char* test2 = malloc(0x20);
    memcpy(test2,"test malloc2",sizeof("test malloc2"));
    uart_puts("%s\n",test2);

    char* test3 = malloc(0x28);
    memcpy(test3,"test malloc3",sizeof("test malloc3"));
    uart_puts("%s\n",test3);
}

void do_cmd_dtb()
{
    traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
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
        //if parse header error
        if(error)
        {
            uart_puts("cpio parse error");
            break;
        }
        // core_timer_enable(2);
        if(strcmp(c_filepath, filepath)==0)
        {
            //exec c_filedata
            char* ustack = malloc(USTACK_SIZE);
            asm("mov x1, 0x3c0\n\t"
                "msr spsr_el1, x1\n\t" // enable interrupt (PSTATE.DAIF) -> spsr_el1[9:6]=4b0. In Basic#1 sample, EL1 interrupt is disabled.
                "msr elr_el1, %0\n\t"   // elr_el1: Set the address to return to: c_filedata
                "msr sp_el0, %1\n\t"    // user program stack pointer set to new stack.
                "eret\n\t"              // Perform exception return. EL1 -> EL0
                :: "r" (c_filedata),
                   "r" (ustack+USTACK_SIZE));
            break;
        }

        //if this is TRAILER!!! (last of file)
        if(header_ptr==0) uart_puts("exec: %s: No such file or directory\n", filepath);
    }

}

void do_cmd_setTimeout(char* msg, char* sec)
{
    add_timer(uart_sendline,atoi(sec),msg);
}

void do_cmd_set2sAlert()
{
    add_timer(timer_set2sAlert,2,"2sAlert");
}

void do_cmd_memory_tester()
{

    char *a = kmalloc(0x10000);
    char *aa = kmalloc(0x10000);
    char *aaa = kmalloc(0x10000);
    char *aaaa = kmalloc(0x10000);
    char *b = kmalloc(0x4000);
    char *c = kmalloc(0x1001); //malloc size = 8KB
    char *d = kmalloc(0x10);   //malloc size = 32B
    // char *e = kmalloc(0x800);   
    // char *f = kmalloc(0x800);   
    // char *g = kmalloc(0x800);   

    kfree(a);
    kfree(aa);
    kfree(aaa);
    kfree(aaaa);
    kfree(b);
    kfree(c);
    kfree(d);

}

void do_cmd_reboot()
{
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}

