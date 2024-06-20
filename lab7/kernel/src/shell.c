#include "shell.h"
#include "mbox.h"
#include "power.h"
#include "stdio.h"
#include "stddef.h"
#include "string.h"
#include "cpio.h"
#include "memory.h"
#include "dtb.h"
#include "timer.h"
#include "uart1.h"
#include "ANSI.h"
#include "sched.h"
#include "syscall.h"
#include "vfs.h"
#include "callback_adapter.h"
#include "vfs.h"

struct CLI_CMDS cmd_list[CLI_MAX_CMD] = {
    {.command = "cat", .help = "concatenate files and print on the standard output", .func = do_cmd_cat},
    {.command = "cd", .help = "change the shell working directory", .func = do_cmd_cd},
    {.command = "dtb", .help = "show device tree", .func = do_cmd_dtb},
    {.command = "exec", .help = "execute a command, replacing current image with a new image", .func = do_cmd_exec},
    {.command = "hello", .help = "print Hello World!", .func = do_cmd_hello},
    {.command = "help", .help = "print all available commands", .func = do_cmd_help},
    {.command = "info", .help = "get device information via mailbox", .func = do_cmd_info},
    {.command = "kmalloc", .help = "test kmalloc", .func = do_cmd_kmalloc},
    {.command = "ls", .help = "list directory contents", .func = do_cmd_ls},
    {.command = "mkdir", .help = "make directories", .func = do_cmd_mkdir},
    {.command = "ps", .help = "print all threads", .func = do_cmd_ps},
    {.command = "setTimeout", .help = "setTimeout [MESSAGE] [SECONDS]", .func = do_cmd_setTimeout},
    {.command = "set2sAlert", .help = "set core timer interrupt every 2 second", .func = do_cmd_set2sAlert},
    {.command = "write", .help = "write to a file", .func = do_cmd_write},
    {.command = "reboot", .help = "reboot the device", .func = do_cmd_reboot},
};

extern void *CPIO_START;
extern thread_t *curr_thread;

int start_shell()
{
    char input_buffer[CMD_MAX_LEN] = {0};
    char path_buf[MAX_PATH_NAME];

#if _DEBUG < 3
    cli_print_banner();
    print_log_level();
#endif
    while (1)
    {
        cli_flush_buffer(input_buffer, CMD_MAX_LEN);
        get_pwd(path_buf);
#if _DEBUG < 3
#ifdef QEMU
        printf("[ " BLU "─=≡Σ((( つ•̀ω•́)つ " GRN "@ QEMU " MAG "%s" CRESET " ] $ ", path_buf);
#elif RPI
        printf("[ " HBLU "d[^_^]b " HGRN "@ RPI " MAG "%s" CRESET " ] $ ", path_buf);
#endif
#endif
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
    return 0;
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
        if (c == 127) // backspace
        {
            if (idx != 0)
            {
                puts("\b \b");
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
            buffer[idx++] = c;
        }
    }
    buffer[idx] = '\0';
    puts("\r\n");
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
    puts("                                                      \r\n");
    puts("                   _oo0oo_                            \r\n");
    puts("                  o8888888o                           \r\n");
    puts("                  88\" . \"88                         \r\n");
    puts("                  (| -_- |)                           \r\n");
    puts("                  0\\  =  /0                          \r\n");
    puts("                ___/`---'\\___                        \r\n");
    puts("              .' \\\\|     |// '.                _____     _ _ _____         _                  \r\n");
    puts("             / \\\\|||  :  |||// \\              |   __|___| | |     |___ ___| |___             \r\n");
    puts("            / _||||| -:- |||||- \\             |   __| .'| | | | | | .'| . | | -_|              \r\n");
    puts("           |   | \\\\\\  -  /// |   |            |__|  |__,|_|_|_|_|_|__,|  _|_|___|            \r\n");
    puts("           | \\_|  ''\\---/''  |_/ |                                    |_|                     \r\n");
    puts("           \\  .-\\__  '-'  ___/-. /            https://github.com/HiFallMaple                  \r\n");
    puts("         ___'. .'  /--.--\\  `. .'___                 \r\n");
    puts("      .\"\" '<  `.___\\_<|>_/___.' >' \"\".           \r\n");
    puts("     | | :  `- \\`.;`\\ _ /`;.`/ - ` : | |            \r\n");
    puts("     \\  \\ `_.   \\_ __\\ /__ _/   .-` /  /          \r\n");
    puts(" =====`-.____`.___ \\_____/___.-`___.-'=====          \r\n");
    puts("                   `=---='                            \r\n");
    puts("\r\n\r\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   \r\n");
    puts("           May the code be bug-free.                 \r\n\r\n");
    puts("\r\n");
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

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((uint64_t)&pt)))
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

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((uint64_t)&pt)))
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
    char *workdir;

    if (argc == 0)
    {
        workdir = ".";
    }
    else
    {
        workdir = argv[0];
    }

    char buf[MAX_NAME_BUF] = {0};
    DEBUG("root_vnode: 0x%x, curr_thread->pwd: 0x%x\r\n", get_root_vnode(), curr_thread->pwd);

    vfs_readdir(curr_thread->pwd, workdir, buf);
    DEBUG("readdir:\r\n");
    int start_index = 0;
    int end_index = 0;
    while (!(buf[end_index] == 0 && buf[end_index + 1] == 0))
    {
        end_index += strlen(buf + start_index) + 1;
        switch (buf[start_index])
        {
        case FS_DIR:
            printf(BBLU "%s " CRESET, buf + start_index + 1);
            break;
        case FS_FILE:
            printf("%s ", buf + start_index + 1);
            break;
        case FS_DEV:
            printf(BCYN "%s " CRESET, buf + start_index + 1);
            break;
        }

        // printf("%s ", buf + start_index);
        start_index = end_index;
    }
    puts("\r\n");

    return 0;
}

int do_cmd_cat(int argc, char **argv)
{
    char *filepath;
    // char *c_filedata;
    // unsigned int c_filesize;

    if (argc == 1)
    {
        filepath = argv[0];
    }
    else
    {
        puts("Incorrect number of parameters\r\n");
        return -1;
    }
    char buf[0x10000] = {0};
    file_t *file;
    if (vfs_open(curr_thread->pwd, filepath, 0, &file) == -1)
        return -1;
    size_t size = vfs_read(file, buf, 0x10000);
    for (int i = 0; i < size; i++)
        printf("%c", buf[i]);
    vfs_close(file);
    return 0;
}

int do_cmd_cd(int argc, char **argv)
{
    char *filepath;
    // char *c_filedata;
    // unsigned int c_filesize;

    if (argc == 1)
    {
        filepath = argv[0];
    }
    else
    {
        puts("Incorrect number of parameters\r\n");
        return -1;
    }
    kernel_chdir(filepath);
}

int do_cmd_kmalloc(int argc, char **argv)
{
    // test kmalloc
    char *test1 = kmalloc(0x18);
    strcpy(test1, "test kmalloc1");
    puts(test1);
    puts("\r\n");

    char *test2 = kmalloc(0x20);
    strcpy(test2, "test kmalloc2");
    puts(test2);
    puts("\r\n");

    char *test3 = kmalloc(0x28);
    strcpy(test3, "test kmalloc3");
    puts(test3);
    puts("\r\n");
    return 0;
}

int do_cmd_dtb(int argc, char **argv)
{
    traverse_device_tree(dtb_callback_show_tree);
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
        puts("Incorrect number of parameters\r\n");
        return -1;
    }
    vnode_t *vnode;
    if (vfs_lookup(curr_thread->pwd, filepath, &vnode) == -1)
    {
        puts("exec: ");
        puts(filepath);
        puts(": No such file or directory\r\n");
        return -1;
    }
    INFO("exec: %s\r\n", filepath);
    if (kernel_fork() == 0)
    { // child process
        kernel_exec_user_program(filepath, NULL);
    }
    else
    {
        wait();
    }
    return 0;
}

int do_cmd_setTimeout(int argc, char **argv)
{
    puts_args_struct_t *args_struct = kmalloc(sizeof(puts_args_struct_t));
    int64_t sec;
    if (argc == 2)
    {
        args_struct->str = kmalloc(strlen(argv[0]) + 1);
        strcpy(args_struct->str, argv[0]);
        sec = atoi(argv[1]);
    }
    else
    {
        puts("Incorrect number of parameters\r\n");
        return -1;
    }
    add_timer_by_sec(sec, adapter_puts, args_struct);
    return 0;
}

int do_cmd_set2sAlert(int argc, char **argv)
{
    if (argc != 0)
    {
        puts("Incorrect number of parameters\r\n");
        return -1;
    }
    add_timer_by_sec(2, adapter_timer_set2sAlert, NULL);
    return 0;
}

int do_cmd_ps(int argc, char **argv)
{
    if (argc != 0)
    {
        puts("Incorrect number of parameters\r\n");
        return -1;
    }
    dump_run_queue();
    return 0;
}

int do_cmd_write(int argc, char **argv)
{
    char *filepath;
    char *content;
    if (argc == 2)
    {
        filepath = argv[0];
        content = argv[1];
    }
    else
    {
        puts("Incorrect number of parameters\r\n");
        return -1;
    }
    file_t *file;
    if (vfs_open(curr_thread->pwd, filepath, 0, &file) == -1)
    {
        puts("File not found\r\n");
        return -1;
    }
    size_t size = strlen(content);
    vfs_write(file, content, size);
    vfs_close(file);
    return 0;
}

int do_cmd_mkdir(int argc, char **argv)
{
    char *dirpath;
    if (argc == 1)
    {
        dirpath = argv[0];
    }
    else
    {
        puts("Incorrect number of parameters\r\n");
        return -1;
    }
    if (vfs_mkdir(curr_thread->pwd, dirpath))
    {
        puts("mkdir failed\r\n");
        return -1;
    }
    return 0;
}
