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
#include "stdint.h"

#define CLI_MAX_CMD 15
#define USTACK_SIZE 0x10000

extern char* dtb_ptr;
extern void* CPIO_DEFAULT_START;

struct CLI_CMDS cmd_list[CLI_MAX_CMD]=
{
    {.command="cat", .help="concatenate files and print on the standard output"},
    {.command="dtb", .help="show device tree"},
    {.command="exec", .help="execute a command, replacing current image with a new image"},
    {.command="hello", .help="print Hello World!"},
    {.command="help", .help="print all available commands"},
    {.command="s_allocator", .help="simple allocator in heap session"},
    {.command="info", .help="get device information via mailbox"},
    {.command="ls", .help="list directory contents"},
    {.command="memory_tester", .help="memory testcase generator, allocate and free"},
    {.command="setTimeout", .help="setTimeout [MESSAGE] [SECONDS]"},
    {.command = "set2sAlert", .help = "set core timer interrupt every 2 second"},
    {.command = "kmalloc", .help = "allocate memory using buddy system and dynamic allocator"},
    {.command = "kfree", .help = "free memory using buddy system and dynamic allocator"},
    {.command = "set2sAlert", .help = "set core timer interrupt every 2 second"},
    {.command = "reboot", .help = "reboot the device"}
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
    char c = '\0';
    int idx = 0;
    while (1)
    {
        if (idx >= CMD_MAX_LEN) break;
        c = uart_async_getc();
        if (c == '\n') break;

        if (c == 127 && idx > 0)
        {
            uart_puts("\b \b");
            idx--;
            continue;
        }
        if (c == 127 && idx == 0) continue;
        
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
    } else if (strcmp(cmd, "s_allocator") == 0) {
        do_cmd_s_allocator();
    } else if (strcmp(cmd, "ls") == 0) {
        do_cmd_ls(argvs);
    } else if (strcmp(cmd, "memory_tester") == 0) {
        do_cmd_memory_tester();
    } else if (strcmp(cmd, "setTimeout") == 0) {
        char* sec = str_SepbySpace(argvs);
        do_cmd_setTimeout(argvs, sec);
    } else if (strcmp(cmd, "set2sAlert") == 0)
    {
        do_cmd_set2sAlert();
    } else if (strcmp(cmd, "kmalloc") == 0){
        do_cmd_kmalloc(argvs);
    } else if (strcmp(cmd, "kfree") == 0)
    {
        do_cmd_kfree(argvs);
    
    } else if (strcmp(cmd, "page_addr"))
    {
        do_cmd_page_addr();
    } else if (strcmp(cmd, "chunk_addr"))
    {
        do_cmd_chunk_addr();
    } else if (strcmp(cmd, "reboot") == 0)
    {
        do_cmd_reboot();
    }
    else{
        uart_puts("Command not found! Type <help> for commands.\n\n");
    }
}

void cli_print_banner() {
    uart_puts("\r\n");
    uart_puts("=======================================\r\n");
    uart_puts("      Welcome to RPI Terminal!         \r\n");
    uart_puts("=======================================\r\n");
    uart_puts(" Type 'help' to see available commands \r\n");
    uart_puts("=======================================\r\n");
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

void do_cmd_dtb()
{
    traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
}

void do_cmd_help() {
    uart_puts("\r\nAvailable Commands:\r\n");
    uart_puts("====================\r\n");

    int max_cmd_length = 0;
    for (int i = 0; i < CLI_MAX_CMD; i++) {
        int cmd_length = strlen(cmd_list[i].command);
        if (cmd_length > max_cmd_length) {
            max_cmd_length = cmd_length;
        }
    }

    for (int i = 0; i < CLI_MAX_CMD; i++) {
        uart_puts(cmd_list[i].command);
        
        int current_length = strlen(cmd_list[i].command);
        int spaces_to_add = max_cmd_length - current_length + 4;
        for (int j = 0; j < spaces_to_add; j++) {
            uart_puts(" ");
        }
        
        uart_puts(": ");
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
        //if parse header error
        if(error)
        {
            uart_puts("cpio parse error");
            break;
        }

        if(strcmp(c_filepath, filepath)==0)
        {
            //exec c_filedata
            char* ustack = s_allocator(USTACK_SIZE);
            asm("msr elr_el1, %0\n\t"   // elr_el1: Set the address to return to: c_filedata
                "msr spsr_el1, xzr\n\t" // enable interrupt (PSTATE.DAIF) -> spsr_el1[9:6]=4b0. In Basic#1 sample, EL1 interrupt is disabled.
                "msr sp_el0, %1\n\t"    // user program stack pointer set to new stack.
                "eret\n\t"              // Perform exception return. EL1 -> EL0
                :: "r" (c_filedata),
                   "r" (ustack+USTACK_SIZE));
            s_free(ustack);
            break;
        }

        //if this is TRAILER!!! (last of file)
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

void do_cmd_s_allocator()
{
    //test malloc
    char* test1 = s_allocator(0x18);
    memcpy(test1,"test malloc1",sizeof("test malloc1"));
    uart_puts("%s\n",test1);

    char* test2 = s_allocator(0x20);
    memcpy(test2,"test malloc2",sizeof("test malloc2"));
    uart_puts("%s\n",test2);

    char* test3 = s_allocator(0x28);
    memcpy(test3,"test malloc3",sizeof("test malloc3"));
    uart_puts("%s\n",test3);
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

void do_cmd_memory_tester()
{
/*
    char *p1 = kmalloc(0x820);
    char *p2 = kmalloc(0x900);
    char *p3 = kmalloc(0x2000);
    char *p4 = kmalloc(0x3900);
    kfree(p3);
    kfree(p4);
    kfree(p1);
    kfree(p2);
*/
    char *a = kmalloc(0x10);
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
}

void do_cmd_setTimeout(char* msg, char* sec)
{
    add_timer(uart_sendline,atoi(sec),msg);
}

void do_cmd_set2sAlert()
{
    add_timer(timer_set2sAlert,2,"2sAlert");
}

void do_cmd_kmalloc(char* size) {
    uart_puts("size (Bytes): ");
    cli_cmd_read(size);
    void* address = kmalloc((unsigned int)atoi(size));
    uintptr_t address_int = (uintptr_t)address;
    
    uart_puts("address: 0x");
    uart_2hex((unsigned int)address_int);
    uart_puts("\n");
}

void do_cmd_kfree(char* addr) {
    uart_puts("address: ");
    cli_cmd_read(addr);
    kfree((void*)str_to_hex(addr));
    uart_puts("\n");
}
// void do_cmd_buddy_system_alloc(char *size)
// {
//     uart_puts("size(KB): ");
//     cli_cmd_read(size);
//     unsigned long int address = buddy_system_alloc(atoi(size));
//     uart_puts("address: 0x");
//     uart_2hex(address);
//     uart_puts("\n");
// }

// void do_cmd_buddy_system_free(char *index)
// {
//     uart_puts("index: ");
//     cli_cmd_read(index);
//     //uart_sendline("atoi :%d\n", hex2dec(index));
//     buddy_system_free(atoi(index));
//     uart_puts("\n");
// }

void do_cmd_page_addr()
{
    print_allocated_pages_addr();
}

void do_cmd_chunk_addr()
{
    print_allocated_chunks_addr();
}

void do_cmd_reboot()
{
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}

