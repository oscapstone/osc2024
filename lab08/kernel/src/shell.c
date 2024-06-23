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
static void rootfs_ls(int argc, char *argv[]);
static void rootfs_mkdir(int argc, char *argv[]);
static void print_cwd(int argc, char *argv[]);
static void change_dir(int argc, char *argv[]);
static void rootfs_touch(int argc, char *argv[]);
static void rootfs_exec(int argc, char *argv[]);


extern void cpio_list(int argc, char *argv[]);
extern void cpio_cat(int argc, char *argv[]);
extern void cpio_exec(int argc, char *argv[]);
extern void print_flist(int argc, char *argv[]);
extern void start_video(int argc, char *argv[]);
extern void print_flist(int argc, char *argv[]);
extern void multiple_thread_test(int argc, char *argv[]);
extern void user_fork_test(int argc, char *argv[]);
extern void user_open_test(int argc, char *argv[]);
extern void user_open_test_initramfs(int argc, char *argv[]);
extern void user_read_test(int argc, char *argv[]);
extern void user_write_test(int argc, char *argv[]);
extern void user_write_test_initramfs(int argc, char *argv[]);
extern void user_stdout_test(int argc, char *argv[]);
extern void user_stdin_test(int argc, char *argv[]);


int split_command(char* command, char *argv[]);

cmd cmds[] = 
{
    {.name = "hello",   .func = &hello,      .help_msg = "\nhello\t: print Hello, World!"},
    {.name = "help",    .func = &help,      .help_msg = "\nhelp\t: print this help menu"},
    {.name = "mailbox", .func = &mailbox,    .help_msg = "\nmailbox\t: print mailbox info"},
    {.name = "ls",     .func = &rootfs_ls, .help_msg = "\nls\t: list files in the rootfs"},
    {.name = "touch",  .func = &rootfs_touch,  .help_msg = "\ntouch\t: create a file in the rootfs"},
    {.name = "mkdir", .func = &rootfs_mkdir, .help_msg = "\nmkdir\t: create a directory in the rootfs"},
    {.name = "cpio_ls",      .func = &cpio_list,  .help_msg = "\nls\t: list files in the cpio archive"},
    {.name = "cpio_cat",     .func = &cpio_cat,   .help_msg = "\ncat\t: print file content in the cpio archive"},
    {.name = "clear",   .func = &clear,      .help_msg = "\nclear\t: clear the screen"},
    {.name = "s_malloc",  .func = &test_malloc,.help_msg = "\nmalloc\t: simple malloc function"},
    {.name = "reboot",  .func = &reboot,     .help_msg = "\nreboot\t: reboot the device"},
    {.name = "cpio_exec",    .func = &cpio_exec,  .help_msg = "\nexec\t: execute a file in the cpio archive"},
    {.name = "multi_thread", .func = &multiple_thread_test, .help_msg = "\nmulti_thread\t: test multiple threads"},
    {.name = "user_fork", .func = &user_fork_test, .help_msg = "\nto_user\t: test user mode"},
    {.name = "user_open", .func = &user_open_test, .help_msg = "\nuser_open\t: test user open file"},
    {.name = "user_open_ramfs", .func = &user_open_test_initramfs, .help_msg = "\nuser_open_ramfs\t: test user open file in initramfs"},
    {.name = "user_read", .func = &user_read_test, .help_msg = "\nuser_read\t: test user read file"},
    {.name = "user_write", .func = &user_write_test, .help_msg = "\nuser_write\t: test user write file"},
    {.name = "user_write_ramfs", .func = &user_write_test_initramfs, .help_msg = "\nuser_write_ramfs\t: test user write file in initramfs"},
    {.name = "user_stdout", .func = &user_stdout_test, .help_msg = "\nuser_stdout\t: test user stdout"},
    {.name = "user_stdin", .func = &user_stdin_test, .help_msg = "\nuser_stdin\t: test user stdin"},
    {.name = "start_video", .func = &start_video, .help_msg = "\nstart_video\t: start video"},
    {.name = "fl", .func= &print_flist, .help_msg = "\nfl\t: print free list"},
    {.name = "cwd", .func=&print_cwd, .help_msg = "\ncwd\t: print current workding directroy"},
    {.name = "cd", .func=&change_dir, .help_msg = "\ncd\t: change the directory"},
    {.name = "exec", .func=&rootfs_exec, .help_msg = "\nexec\t: execute a file in the rootfs"}
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
        vfs_list(current->cwd);
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

static void rootfs_touch(int argc, char *argv[])
{
    if(argc == 2){
        // vfs_create(argv[1]);
    }
    else{
        printf("\nUsage: touch [path]");
        return;
    }
}

static void rootfs_exec(int argc, char *argv[])
{
    if(argc == 2){
        vfs_exec(argv[1], 0);
    }
    else{
        printf("\nUsage: exec [path]");
        return;
    }
}


static void print_cwd(int argc, char *argv[])
{
    printf("\nCurrent Working Directory: ");
    printf(current->cwd);
}



static void change_dir(int argc, char *argv[])
{
    if(argc == 2){
        vfs_chdir(argv[1]);
    }
    else{
        printf("\nUsage: cd [path]");
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