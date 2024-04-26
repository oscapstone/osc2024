#include "shell.h"
#include "mini_uart.h"
#include "mailbox.h"
#include "reboot.h"
#include "io.h"
#include "string.h"
#include "alloc.h"
#include "lib.h"
#include "timer.h"
#include "irq.h"
#include "alloc.h"

static void hello(int argc, char *argv[]);
static void help(int argc, char *argv[]);
static void clear(int argc, char *argv[]);
static void mailbox(int argc, char *argv[]);
static void test_malloc(int argc, char *argv[]);
static void reboot(int argc, char *argv[]);
static void set_timeout(int argc, char *argv[]);
static void balloc_wrapper(int argc, char *argv[]);
static void dynamic_alloc_wrapper(int argc, char *argv[]);
static void dynamic_free_wrapper(int argc, char *argv[]);

extern void cpio_list(int argc, char *argv[]);
extern void cpio_cat(int argc, char *argv[]);
extern void cpio_exec(int argc, char *argv[]);
extern void print_flist(int argc, char *argv[]);
extern void print_allocated(int argc, char *argv[]);
extern void bfree_wrapper(int argc, char* argv[]);
extern void print_bslist(int argc, char* argv[]);


int split_command(char* command, char *argv[]);

cmd cmds[] = 
{
    {.name = "hello",   .func = &hello,      .help_msg = "\nhello\t: print Hello, World!"},
    {.name = "help",    .func = &help,      .help_msg = "\nhelp\t: print this help menu"},
    {.name = "mailbox", .func = &mailbox,    .help_msg = "\nmailbox\t: print mailbox info"},
    {.name = "ls",      .func = &cpio_list,  .help_msg = "\nls\t: list files in the cpio archive"},
    {.name = "cat",     .func = &cpio_cat,   .help_msg = "\ncat\t: print file content in the cpio archive"},
    {.name = "clear",   .func = &clear,      .help_msg = "\nclear\t: clear the screen"},
    {.name = "s_malloc",  .func = &test_malloc,.help_msg = "\nmalloc\t: simple malloc function"},
    {.name = "reboot",  .func = &reboot,     .help_msg = "\nreboot\t: reboot the device"},
    {.name = "exec",    .func = &cpio_exec,  .help_msg = "\nexec\t: execute a file in the cpio archive"},
    {.name = "async",   .func = &async_shell, .help_msg = "\nasync\t: enter dummy async shell mode"},
    {.name = "set_timeout", .func = &set_timeout, .help_msg = "\nset_timeout\t: set a new timer"},
    {.name = "print_flist", .func = &print_flist, .help_msg = "\nprint_flist\t: print free list"},
    {.name = "print_allocated", .func = &print_allocated, .help_msg = "\nprint_allocated\t: print allocated frames"},
    {.name = "balloc", .func = &balloc_wrapper, .help_msg = "\nb_malloc\t: buddy system allocation"},
    {.name = "bfree", .func = &bfree_wrapper, .help_msg = "\nbfree\t: buddy system free"},
    {.name = "dalloc", .func = &dynamic_alloc_wrapper, .help_msg = "\ndalloc\t: dynamic allocation"},
    {.name = "dfree", .func= &dynamic_free_wrapper, .help_msg = "\ndfree\t: dynamic free"},
    {.name = "print_bslist", .func = &print_bslist, .help_msg = "\nprint_bslist\t: print buddy system list"}
};


void shell()
{
    printf("\nyuchang@raspberrypi3: ~$ ");
    char command[256];
    readcmd(command);
    
    if(command[0] == 0)return;

    char* argv[16];
    int argc = split_command(command, argv);
    
    for(int i=0; i<sizeof(cmds)/sizeof(cmd); i++)
    {
        // if(strcmp(command, cmds[i].name) == 0)
        if(strcmp(argv[0], cmds[i].name) == 0)
        {
            // char* dummy_argv[1];
            cmds[i].func(argc, argv);
            return;
        }
    }
    printf("\nCommand not found: ");
    printf(command);
}

void async_shell()
{
    enable_uart_interrupt();
    char command[BUFF_SIZE];
    while(1)
    {
        printf("\r\n(async) yuchang@raspberrypi3: ~$ ");
        async_uart_gets(command, BUFF_SIZE);
        async_uart_puts(command);        
        if(strcmp(command, "exit") == 0)
        {
            printf("\r\nExiting async shell...");
            break;
        }
        else if(strcmp(command, "help") == 0)
        {
            printf("\r\nhelp\t: print this help menu");
            printf("\r\nexit\t: exit async shell");
        }
    }
    disable_uart_interrupt();
}

/* ===================================================================== */


static void hello(int argc, char **argv)
{
    printf("\nHello, World!");
}

static void help(int argc, char **argv)
{
    for(int i=0; i<sizeof(cmds)/sizeof(cmd); i++)
    {
        printf(cmds[i].help_msg);
    }
}

static void clear(int argc, char **argv)
{
    printf("\033[H\033[J");
}

static void mailbox(int argc, char **argv)
{
    printf("\nMailbox info:");
    get_board_revision();
    get_memory_info();
}

static void test_malloc(int argc, char **argv)
{
    printf("\n1. char* p = simple_malloc(10);");
    char* p = simple_malloc(10);
    strcpy(p, "123456789");
    printf("\nMemory content: ");
    for(int i=0; i<10; i++){
        printfc(p[i]);
    }

    printf("\n2. char* p2 = simple_malloc(20);");
    char* p2 = simple_malloc(20);
    strcpy(p2, "1122334455667788990");
    printf("\nMemory content: ");
    for(int i=0; i<20; i++){
        printfc(p2[i]);
    }
}


static void set_timeout(int argc, char *argv[])
{
    if(argc != 3){
        printf("\nUsage: set_timeout <msg> <sec>");
        return;
    }
    char* msg = argv[1];
    unsigned int sec = atoi(argv[2]);
    add_timer(print_timeout_msg, msg, sec);
    core_timer_enable();
}

static void reboot(int argc, char **argv)
{
    printf("\nRebooting...\n");
    reset(200);
}

// static void* alloc_queue[10] = {0};
// static uint32_t alloc_queue_front = 0;
// static uint32_t alloc_queue_rear = 0;

static void balloc_wrapper(int argc, char **argv)
{
    if(argc != 2){
        printf("\nUsage: balloc_wrapper <size>");
        return;
    }
    uint64_t size = atoi(argv[1]);
    void* pt = balloc(size);
    if(pt == 0){
        printf("\nAllocation failed");
        return;
    }
    printf("\nAllocated at: "); printf_hex((uint64_t)pt);
    printf("\n===============================");
}

static void dynamic_alloc_wrapper(int argc, char **argv)
{
    if(argc != 2){
        printf("\nUsage: dynamic_alloc_wrapper <size>");
        return;
    }
    uint64_t size = atoi(argv[1]);
    void* pt = dynamic_alloc(size);
    if(pt == 0){
        printf("\nAllocation failed");
        return;
    }
    printf("\nAllocated at: "); printf_hex((uint64_t)pt);
    printf("\n===============================");
}

static void dynamic_free_wrapper(int argc, char *argv[])
{
    if(argc != 2){
        printf("\nUsage: dynamic_free_wrapper <addr>");
        return;
    }
    void* ptr = (void*)atoi_hex(argv[1]);
    if(dfree(ptr)){
        printf("\nFree failed");
        return;
    }
    printf("\nFree success");

}

void readcmd(char x[256])
{
    char input_char;
    int input_index = 0;
    x[0] = 0;
    while( ((input_char = read_char()) != '\n'))
    {
        if(input_char == 127){
            if(input_index > 0){
                input_index--;
                printf("\b \b");
            }
            continue;
        }
        x[input_index] = input_char;
        ++input_index;
        printfc(input_char);
    }

    x[input_index]=0; // null char
}

int split_command(char* command, char *argv[])
{
    int argc = 0;

    char *token = strtok(command, " ");
    while(*token != '\0')
    {
        argv[argc] = token;
        argc++;
        token = strtok(0, " ");
    }
    return argc;
}