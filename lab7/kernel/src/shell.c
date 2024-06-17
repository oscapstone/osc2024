#include "shell.h"
#include "mbox.h"
#include "power.h"
#include "stdio.h"
#include "string.h"
#include "cpio.h"
#include "memory.h"
#include "dtb.h"
#include "timer.h"
#include "sched.h"
#include "signal.h"
#include "syscall.h"
#include "vfs.h"
#include "vfs_tmpfs.h"
#include "debug.h"

#include "uart1.h"

#define USTACK_SIZE 0x10000

extern unsigned long long int lock_counter;
extern list_head_t *run_queue;
extern execfile c_execfile;
extern struct mount *rootfs;

struct CLI_CMDS cmd_list[CLI_MAX_CMD] = {
    {.command = "cat", .help = "concatenate files and print on the standard output", .func = do_cmd_cat},
    {.command = "dtb", .help = "show device tree", .func = do_cmd_dtb},
    {.command = "hello", .help = "print Hello World!", .func = do_cmd_hello},
    {.command = "help", .help = "print all available commands", .func = do_cmd_help},
    {.command = "info", .help = "get device information via mailbox", .func = do_cmd_info},
    {.command = "malloc", .help = "test malloc", .func = do_cmd_malloc},
    {.command = "reboot", .help = "reboot the device", .func = do_cmd_reboot},
    {.command = "exec", .help = "execute user programs ", .func = do_cmd_exec},
    {.command = "setTime", .help = "setTime [MESSAGE] [SECONDS] ", .func = do_cmd_setTimeout},
    {.command = "2sAlert", .help = "set core timer interrupt every 2 second ", .func = do_cmd_set2sAlert},
    {.command = "mtest", .help = "memory testcase generator, allocate and free", .func = do_cmd_mtest},
    {.command = "ttest", .help = "thread tester with dummy function - foo()", .func = do_cmd_ttest},
    {.command = "ls", .help = "list directory contents", .func = do_cmd_ls},
    {.command = "cd", .help = "change directory", .func = do_cmd_cd},
    {.command = "ftest", .help = "get run_queue size", .func = do_cmd_ftest}};

extern char *dtb_ptr;
extern thread_t *curr_thread;
extern void *CPIO_DEFAULT_START;

void start_shell()
{
    // uart_sendlinek("In start_shell\n");
    char input_buffer[CMD_MAX_LEN];

    cli_print_banner();
    while (1)
    {
        cli_flush_buffer(input_buffer, CMD_MAX_LEN);
        // uart_sendlinek("lock_counter : %d\n",lock_counter);
        puts("【 Ciallo～(∠・ω< )⌒★ 】 # ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
        idle();
        // uart_sendlinek("idle finish\n");
    }
    // return 0;
}

void cli_flush_buffer(char *buffer, int length)
{
    for (int i = 0; i < length; i++)
    {
        buffer[i] = '\0';
    }
};

void cli_cmd_read(char *buffer)
{
    char c = '\0';
    int idx = 0;

    while (idx < CMD_MAX_LEN - 1)
    {
        c = getchar();
        // vfs_read(uart, c, 0);

        if (c == 127) // backspace
        {
            if (idx != 0)
            {
                puts("\b \b");
                // vfs_write("/dev/uart","\b \b",0);
                idx--;
            }
        }
        else if (c == '\n')
        {
            break;
        }
        else if (c <= 16 || c >= 32 || c < 127)
        {
            putchar(c);
            // vfs_write("/dev/uart","\b \b",0);
            buffer[idx++] = c;
        }
    }
    buffer[idx] = '\0';
    puts("\r\n");
    // vfs_write("/dev/uart","\r\n",0);
}

int _parse_args(char *buffer, int *argc, char **argv)
{
    char get_cmd = 0;
    for (int i = 0; buffer[i] != '\0'; i++)
    {
        if (!get_cmd)
        {
            if (buffer[i] == ' ')
            {
                buffer[i] = '\0';
                get_cmd = 1;
            }
        }
        else
        {
            if (buffer[i - 1] == '\0' && buffer[i] != ' ' && buffer[i] != '\0')
            {
                if (*argc >= CMD_MAX_PARAM)
                {
                    return -1;
                }
                argv[*argc] = buffer + i;
                (*argc)++;
            }
            else if (buffer[i] == ' ')
            {
                buffer[i] = '\0';
            }
        }
    }
    return 0;
}

void print_args(int argc, char **argv)
{
    puts("argc: ");
    put_int(argc);
    puts("\r\n");
    for (int i = 0; i < argc; i++)
    {
        puts("argv[");
        put_int(i);
        puts("]: ");
        puts(argv[i]);
        puts("\r\n");
    }
}

void cli_cmd_exec(char *buffer)
{
    char *cmd = buffer;
    int argc = 0;
    char *argv[CMD_MAX_PARAM];
    if (_parse_args(buffer, &argc, argv) == -1)
    {
        puts("Too many arguments\r\n");
        return;
    }
    // print_args(argc, argv);

    for (int i = 0; i < CLI_MAX_CMD; i++)
    {
        if (strcmp(cmd, cmd_list[i].command) == 0)
        {
            cmd_list[i].func(argc, argv);
            return;
        }
    }
    if (*buffer)
    {
        puts(buffer);
        puts(": command not found\r\n");
    }
}

void cli_print_banner()
{
    // uart_sendlinek("In cli_print_banner\n");
    puts("\r\n");
    puts("============================================================================================\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%@@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-@@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ %@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ *@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@# =@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@= :@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-  %@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%   *@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@#%@@@@@@@@@@@@@@@@@@@*   -@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@% *@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@:=@@@@@@@@@@@@@@@@@@@:    %@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@- :@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*  #@@@@@@@@@@@@@@@@@*     -@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@#-   :#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%+    +%@@@@@@@@@@@@@@+       -%@@@@@@@@@@@@\r\n");
    puts("@@@@*=:                                                 :=#@@@@@@@@#=:          =*@@@@@@@@@@\r\n");
    puts("@@@@@#=                                                =*%@@@@@@@@@@*+-       :=*%@@@@@@@@@@\r\n");
    puts("@@@@@@@-      @@@@@@@@@@@#    -@@@@@@@@@@@@@@@@@@%:  -%@%@@@@@@@@@@@@@@%=   :#@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@=      @%++*%@@@@@%:   +@@@@#==#@@@@@@@@@@@%  %*  -%@@#-=+%@@@@@@@-  %@@@@@+::=*%@@@@\r\n");
    puts("@@@@@@@=      @%                        =%@+               #@+                          =%@@\r\n");
    puts("@@@@@@@=      @%      ************-      =@                 #+                          :%@@\r\n");
    puts("@@@@@@@=      @%      @@@@@@@@@@@@+      @@%    #=          +=      %@@@*   =@@@@-     :@@@@\r\n");
    puts("@@@@@@@=      @%      ##########%@+     :@@%    #=    +@@@@@@=      %@#=     -#@@=     -@@@@\r\n");
    puts("@@@@@@@=      @%      :::::::::::@+     :@@%    #=    +#+=++@=      -:         -:      -@@@@\r\n");
    puts("@@@@@@@=      @%      @@@@@@@@@@@@+     :@@%    #=    ++    @=      -           :      -@@@@\r\n");
    puts("@@@@@@@=     :@%      @@@@@@@@@@@@+     :@@%    #=    ++    @=      %@#-     :*%@-     -@@@@\r\n");
    puts("@@@@@@@=     =@%                        :@@%    #=    ++    @=      %@@@+   -@@@@-     -@@@@\r\n");
    puts("@@@@@@@-     #@%     ****:     +**=     +@@%    #=    ++    @=      *####:  #####:     -@@@@\r\n");
    puts("@@@@@@@     :@@%-=*%@@@@@-     #@@#+*#%@@@@%    #=    ++   =@+                         -@@@@\r\n");
    puts("@@@@@@*     %@@@@#*=--@@@-     #@@#-=+*%@@@%    %=    *+  =@@+     +@@@@@= :@@@@@#     -@@@@\r\n");
    puts("@@@@@@:    *@%+-      =@@-     #@#:     :=*+   :@=    +*=%@@@+   =%@@@@@*   -@@@@@@+:  -@@@@\r\n");
    puts("@@@@@=    *#-           *-     #-              *@=    +@@@@@@#+*@@@@@@*-     :+%@@@@@#+*@@@@\r\n");
    puts("@@@@+   -%@:     -=**%@@@-     #@@%#*+=:      *@@=    *@@@@@@@@@@@@=:           :-%@@@@@@@@@\r\n");
    puts("@@@+  -#@@@%-  =@@@@@@@@@-     #@@@@@@@@%   =%@@@=    *@@@@@@@@@@@@@#-         :*%@@@@@@@@@@\r\n");
    puts("@@=:+%@@@@@@@%+--=*%@@@@@-    :@@@@@@#*=-=*@@@@@@=    #@@@@@@@@@@@@@@@*       =@@@@@@@@@@@@@\r\n");
    puts("@%%@@@@@@@@@@@@@@@%##@@@@-    #@@@@%#%@@@@@@@@@@@=   +@@@@@@@@@@@@@@@@@*     -@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@-  =%@@@@@@@@@@@@@@@@@@@=  *@@@@@@@@@@@@@@@@@@@:    %@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@++%@@@@@@@@@@@@@@@@@@@@@*+%@@@@@@@@@@@@@@@@@@@@+   -@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%   +@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   %@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@=  %@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@= :@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@* =@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@% =@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ *@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ #@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ %@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@-%@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@=@@@@@@@@@@@@@@@@@\r\n");
    puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@%@@@@@@@@@@@@@@@@@\r\n");
    puts("============================================================================================\r\n");
}

int do_cmd_help(int argc, char **argv)
{
    for (int i = 0; i < CLI_MAX_CMD; i++)
    {
        puts(cmd_list[i].command);
        puts("\t\t\t: ");
        puts(cmd_list[i].help);
        puts("\r\n");
    }
    return 0;
}

int do_cmd_hello(int argc, char **argv)
{
    puts("Hello World!\r\n");
    return 0;
}

int do_cmd_info(int argc, char **argv)
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

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        puts("Hardware Revision\t: 0x");
        // put_hex(pt[6]);
        put_hex(pt[5]);
        puts("\r\n");
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

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        puts("ARM Memory Base Address\t: 0x");
        put_hex(pt[5]);
        puts("\r\n");
        puts("ARM Memory Size\t\t: 0x");
        put_hex(pt[6]);
        puts("\r\n");
    }
    return 0;
}

int do_cmd_reboot(int argc, char **argv)
{
    if (argc == 0)
    {

        puts("Reboot in 10 seconds ...\r\n\r\n");
        volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
        *rst_addr = PM_PASSWORD | 0x20;
        volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
        *wdg_addr = PM_PASSWORD | 0x70000;
    }
    else if (argc == 1 && strcmp(argv[0], "-c") == 0)
    {
        puts("Cancel reboot...\r\n");
        volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
        *rst_addr = PM_PASSWORD | 0x0;
        volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
        *wdg_addr = PM_PASSWORD | 0x0;
    }
    return 0;
}

int do_cmd_ls(int argc, char **argv)
{
    // char *c_filepath;
    // char *c_filedata;
    // unsigned int c_filesize;

    // CPIO_for_each(&c_filepath, &c_filesize, &c_filedata)
    // {
    //     if (header_ptr != 0)
    //     {
    //         puts(c_filepath);
    //         puts("\r\n");
    //     }
    // }
    vfs_ls();
    return 0;
}

int do_cmd_cat(int argc, char **argv)
{
    int FLAG_getfile = 0;
    char *filepath;
    char *c_filepath;
    char *c_filedata;
    unsigned int c_filesize;

    if (argc == 1)
    {
        filepath = argv[0];
    }
    else
    {
        puts("Too many arguments\r\n");
        return -1;
    }

    CPIO_for_each(&c_filepath, &c_filesize, &c_filedata)
    {
        if (strcmp(c_filepath, filepath) == 0)
        {
            FLAG_getfile = 1;
            Readfile(c_filedata, c_filesize);
            break;
        }
    }

    if (!FLAG_getfile)
    {
        puts("cat: ");
        puts(filepath);
        puts(": No such file or directory\r\n");
    }
    return 0;
}

int do_cmd_malloc(int argc, char **argv)
{
    // test malloc
    char *test1 = allocator(0x18);
    strcpy(test1, "test malloc1");
    puts(test1);
    puts("\r\n");

    char *test2 = allocator(0x20);
    strcpy(test2, "test malloc2");
    puts(test2);
    puts("\r\n");

    char *test3 = allocator(0x28);
    strcpy(test3, "test malloc3");
    puts(test3);
    puts("\r\n");
    return 0;
}

int do_cmd_dtb(int argc, char **argv)
{
    traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
    return 0;
}

int do_cmd_exec(int argc, char **argv)
{
    char *filepath;

    if (argc == 1)
    {
        filepath = argv[0];
    }
    else
    {
        puts("Too many arguments\r\n");
        return -1;
    }

    char abs_path[MAX_PATH_NAME];
    struct vnode *target_file;
    strcpy(abs_path, filepath);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);

    strcpy(abs_path,"/initramfs/vfs1.img");//
    if (vfs_lookup(abs_path, &target_file) != 0)
    {
        WARING("File : %s Does not Exit!!\n", abs_path);
        return 0;
    };

    c_execfile.vnode = target_file;
    c_execfile.pathname = abs_path;
    exec_thread();

    // c_execfile.data = target_file->f_ops;
    // c_execfile.filesize = target_file->f_ops->getsize(target_file);;

    // CPIO_for_each(&c_filepath, &c_filesize, &c_filedata)
    // {
    //     if (strcmp(c_filepath, filepath) == 0)
    //     {
    //         // exec c_filedata
    //         FLAG_getfile = 1;
    //         c_execfile.data = c_filedata;
    //         c_execfile.filesize = c_filesize;
    //         exec_thread();
    //     }
    // }

    // if (!FLAG_getfile) // header_ptr
    // {
    //     puts("cat: ");
    //     puts(filepath);
    //     puts(": No such file or directory\r\n");
    // }
    return 0;
}

int do_cmd_setTimeout(int argc, char **argv)
{
    char *msg;
    int sec;
    if (argc == 2)
    {
        msg = argv[0];
        sec = atoi(argv[1]);
    }
    else
    {
        puts("setTimeout [MESSAGE] [SECONDS]\r\n");
        return -1;
    }
    add_timer(puts, sec, msg, setSecond);
    return 0;
}

int do_cmd_set2sAlert(int argc, char **argv)
{
    add_timer(timer_set2sAlert, 2, "2sAlert", setSecond);
    return 0;
}

int do_cmd_mtest(int argc, char **argv)
{
    // char *a = kmalloc(513);
    // uart_sendlinek("a : %x\n", a);
    // //kfree(a);

    // char *b = kmalloc(512);
    // uart_sendlinek("b : %x\n", b);
    // //kfree(b);

    // char *c = kmalloc(8);
    // //kfree(c);

    return 0;
}

int do_cmd_ttest(int argc, char **argv)
{
    uart_sendlinek("run_queue size : %d\n", list_size(run_queue));
    list_head_t *pos;
    list_for_each(pos, run_queue)
    {
        uart_sendlinek("pid : %d\n", ((thread_t *)pos)->pid);
        for (int i = 0; i < SIGNAL_MAX; i++)
        {
            uart_sendlinek("signal_handler : 0x%x\n", ((thread_t *)pos)->signal_handler[i]);
        }
    }
    return 0;
}

int do_cmd_ftest(int argc, char **argv)
{
    // ============================== test for file operation start==================================
    uart_sendlinek("\n\n");
    uart_sendlinek("+-----------------------------+\n");
    uart_sendlinek("|      mkdir,mount test       |\n");
    uart_sendlinek("+-----------------------------+\n");

    vfs_mkdir("/lll");
    vfs_mkdir("/aaa");
    vfs_mkdir("/bbb");
    vfs_mkdir("/lll/ddd");
    vfs_mount("/lll/ddd", "tmpfs");
    vfs_mkdir("/lll/ddd/fff");
    vfs_dump(rootfs->root, 0);

    uart_sendlinek("\n\n");
    uart_sendlinek("+-----------------------------+\n");
    uart_sendlinek("|    open,write,read test     |\n");
    uart_sendlinek("+-----------------------------+\n");

    struct file *testfilew;
    struct file *testfiler;
    char testbufw[0x30] = "ABCDEABBBBBBDDDDDDDDDDD\n";
    char testbufr[0x30] = {};

    vfs_open("/lll/ddd/ggg.file", O_CREAT, &testfilew);
    vfs_open("/lll/ddd/ggg.file", O_CREAT, &testfiler);
    vfs_write(testfilew, testbufw, 25);
    vfs_read(testfiler, testbufr, 25);

    uart_sendlinek("%s", testbufr);
    vfs_dump(rootfs->root, 0);
    // ============================== test for file operation end ==================================

    // ============================== test for get_absolute_path start==================================
    // char *currt_path = "/root";
    // uart_sendlinek("currt_path : %s\n", currt_path);
    // char *path1 = "/";
    // uart_sendlinek("path1 rel_path : %s\n", path1);
    // get_absolute_path(path1, currt_path);
    // // uart_sendlinek("path1 abs_path : %s\n", path1);
    // uart_sendlinek("---------------------------------------\n");
    // char *path2 = "desktop";
    // uart_sendlinek("path2 rel_path : %s\n", path2);
    // get_absolute_path(path2, currt_path);
    // // uart_sendlinek("path2 abs_path : %s\n", path2);
    // uart_sendlinek("---------------------------------------\n");
    // char *path3 = ".///////";
    // uart_sendlinek("path3 rel_path : %s\n", path3);
    // get_absolute_path(path3, currt_path);
    // // uart_sendlinek("path3 abs_path : %s\n", path3);
    // ============================== test for get_absolute_path end==================================

    // char test[11] = "abcdefghijk";
    // char *t2 = &test[3];
    // uart_sendlinek("abs_path : %s\n", test);
    // uart_sendlinek("abs_path : %s\n", t2);
    // t2 = &test[5];
    // uart_sendlinek("abs_path : %s\n", t2);

    return 0;
}

int do_cmd_cd(int argc, char **argv)
{
    char *filepath;
    if (argc == 1)
    {
        filepath = argv[0];
    }
    else
    {
        puts("Too many arguments\r\n");
        return -1;
    }
    vfs_cd(filepath);
    return 0;
}