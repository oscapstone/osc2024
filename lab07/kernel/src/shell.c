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
#include "fork.h"
#include "schedule.h"
#include "vfs.h"

#define delay(x) for(int i=0; i<x; i++) asm volatile("nop");

static void hello(int argc, char *argv[]);
static void help(int argc, char *argv[]);
static void clear(int argc, char *argv[]);
static void mailbox(int argc, char *argv[]);
static void test_malloc(int argc, char *argv[]);
static void reboot(int argc, char *argv[]);
static void multiple_thread_test(int argc, char *argv[]);
static void to_user_test(int argc, char *argv[]);
static void rootfs_ls(int argc, char *argv[]);
static void rootfs_mkdir(int argc, char *argv[]);

extern void cpio_list(int argc, char *argv[]);
extern void cpio_cat(int argc, char *argv[]);
extern void cpio_exec(int argc, char *argv[]);
extern void print_flist(int argc, char *argv[]);
extern void start_video(int argc, char *argv[]);
extern void print_flist(int argc, char *argv[]);

// system call
extern int getpid();
extern int fork();
extern void exit();

int split_command(char* command, char *argv[]);

cmd cmds[] = 
{
    {.name = "hello",   .func = &hello,      .help_msg = "\nhello\t: print Hello, World!"},
    {.name = "help",    .func = &help,      .help_msg = "\nhelp\t: print this help menu"},
    {.name = "mailbox", .func = &mailbox,    .help_msg = "\nmailbox\t: print mailbox info"},
    {.name = "ls",     .func = &rootfs_ls, .help_msg = "\nls\t: list files in the rootfs"},
    {.name = "mkdir", .func = &rootfs_mkdir, .help_msg = "\nmkdir\t: create a directory in the rootfs"},
    {.name = "cpio_ls",      .func = &cpio_list,  .help_msg = "\nls\t: list files in the cpio archive"},
    {.name = "cpio_cat",     .func = &cpio_cat,   .help_msg = "\ncat\t: print file content in the cpio archive"},
    {.name = "clear",   .func = &clear,      .help_msg = "\nclear\t: clear the screen"},
    {.name = "s_malloc",  .func = &test_malloc,.help_msg = "\nmalloc\t: simple malloc function"},
    {.name = "reboot",  .func = &reboot,     .help_msg = "\nreboot\t: reboot the device"},
    {.name = "exec",    .func = &cpio_exec,  .help_msg = "\nexec\t: execute a file in the cpio archive"},
    {.name = "multi_thread", .func = &multiple_thread_test, .help_msg = "\nmulti_thread\t: test multiple threads"},
    {.name = "to_user", .func = &to_user_test, .help_msg = "\nto_user\t: test user mode"},
    {.name = "start_video", .func = &start_video, .help_msg = "\nstart_video\t: start video"},
    {.name = "fl", .func= &print_flist, .help_msg = "\nfl\t: print free list"}
};

static void shell()
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

void shell_loop()
{
    while(1)
    {
        shell();
    }
}

extern struct task_struct *current;

static void foo()
{
    for(int i = 0; i < 10; ++i) {
        printf("\r\nThread id: "); printf_int(current->pid); printf("\t,loop: "); printf_int(i);
        delay(1000000);
        schedule();
    }
    current->state = TASK_ZOMBIE;
    while(1);
}

static void user_foo()
{
    // printf("Fork Test , pid : %d\n",getpid());
    printf("\r\nFork Test, pid: "); printf_int(getpid());
    uint32_t cnt = 1,ret=0;

    if((ret=fork()) == 0){ //pid == 0 => child
        printf("\r\n===== Child Process =====");
        unsigned long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        // printf("first  child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        printf("\r\nfirst child pid: "); printf_int(getpid()); printf(", cnt: "); printf_int(cnt);
        printf(", ptr: "); printf_hex((unsigned long)&cnt); printf(", sp: "); printf_hex(cur_sp);
        ++cnt;

        if ((ret = fork() )!= 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            // printf("first  child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
            printf("\r\nfirst child pid: "); printf_int(getpid()); printf(", cnt: "); printf_int(cnt);
            printf(", ptr: "); printf_hex((unsigned long)&cnt); printf(", sp: "); printf_hex(cur_sp);
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                // printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
                printf("\r\nsecond child pid: "); printf_int(getpid()); printf(", cnt: "); printf_int(cnt);
                printf(", ptr: "); printf_hex((unsigned long)&cnt); printf(", sp: "); printf_hex(cur_sp);
                delay(1000000);
                ++cnt;
            }
        }
        exit();
    }
    else{ //pid > 0 => parent
        printf("\r\n===== Parent Process =====");
        printf("\r\nParent Process, pid: "); printf_int(getpid());
        printf(" child pid: "); printf_int(ret);
        unsigned long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        printf(" cnt: "); printf_int(cnt); printf(" , ptr: "); printf_hex((unsigned long)&cnt);
        printf(" , sp: "); printf_hex(cur_sp);
        exit();
    }
}

static void multiple_thread_test(int argc, char *argv[])
{
    for(int i = 0; i < 5; ++i) {
        copy_process(PF_KTHREAD, (unsigned long)&foo, 0, 0);
    }
}

static void to_user_test(int argc, char *argv[])
{
    copy_process(PF_KTHREAD, (unsigned long)&kp_user_mode, (unsigned long)&user_foo, 0);
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


static void reboot(int argc, char **argv)
{
    printf("\nRebooting...\n");
    reset(200);
}

static void rootfs_ls(int argc, char *argv[])
{
    if(argc == 1){
        vfs_list("/");
    }
    else if(argc == 2){
        vfs_list(argv[1]);
    }
    else{
        printf("\nUsage: ls [path]");
        return;
    }
}


static void rootfs_mkdir(int argc, char *argv[])
{
    if(argc == 2){
        vfs_mkdir(argv[1]);
    }
    else{
        printf("\nUsage: mkdir [path]");
        return;
    }
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
    while(token != NULL)
    {
        argv[argc] = token;
        argc++;
        token = strtok(0, " ");
    }
    return argc;
}