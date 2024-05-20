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
#include "callback_adapter.h"

struct CLI_CMDS cmd_list[CLI_MAX_CMD] = {
    {.command = "cat", .help = "concatenate files and print on the standard output", .func = do_cmd_cat},
    {.command = "dtb", .help = "show device tree", .func = do_cmd_dtb},
    {.command = "exec", .help = "execute a command, replacing current image with a new image", .func = do_cmd_exec},
    {.command = "hello", .help = "print Hello World!", .func = do_cmd_hello},
    {.command = "help", .help = "print all available commands", .func = do_cmd_help},
    {.command = "info", .help = "get device information via mailbox", .func = do_cmd_info},
    {.command = "kmalloc", .help = "test kmalloc", .func = do_cmd_kmalloc},
    {.command = "ls", .help = "list directory contents", .func = do_cmd_ls},
    {.command = "ps", .help = "print all threads", .func = do_cmd_ps},
    {.command = "setTimeout", .help = "setTimeout [MESSAGE] [SECONDS]", .func = do_cmd_setTimeout},
    {.command = "set2sAlert", .help = "set core timer interrupt every 2 second", .func = do_cmd_set2sAlert},
    {.command = "reboot", .help = "reboot the device", .func = do_cmd_reboot},
};

extern void *CPIO_START;
extern thread_t *curr_thread;

int start_shell()
{
    char input_buffer[CMD_MAX_LEN] = {0};
#if _DEBUG < 3
    cli_print_banner();
    print_log_level();
#endif
    while (1)
    {
        cli_flush_buffer(input_buffer, CMD_MAX_LEN);
#if _DEBUG < 3
#ifdef QEMU
        puts("[ " BLU "─=≡Σ((( つ•̀ω•́)つ " GRN "@ QEMU" CRESET " ] $ ");
#elif RPI
        puts("[ " HBLU "d[^_^]b " HGRN "@ RPI" CRESET " ] $ ");
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
    char *c_filepath;
    char *c_filedata;
    unsigned int c_filesize;

    if (argc == 0)
    {
        workdir = ".";
    }
    else
    {
        workdir = argv[0];
    }
    int error;
    CPIO_FOR_EACH(&c_filepath, &c_filesize, &c_filedata, error, {
        puts(c_filepath);
        puts("\r\n");
    });
    if (error == CPIO_ERROR)
    {
        puts("cpio parse error");
        return -1;
    }
    return 0;
}

int do_cmd_cat(int argc, char **argv)
{
    char *filepath;
    char *c_filedata;
    unsigned int c_filesize;

    if (argc == 1)
    {
        filepath = argv[0];
    }
    else
    {
        puts("Incorrect number of parameters\r\n");
        return -1;
    }

    int result = cpio_get_file(filepath, &c_filesize, &c_filedata);

    if (result == CPIO_ERROR)
    {
        puts("cpio parse error\r\n");
        return -1;
    }
    else if (result == CPIO_TRAILER)
    {
        puts("cat: ");
        puts(filepath);
        puts(": No such file or directory\r\n");
        return -1;
    }
    else if (result == CPIO_SUCCESS)
    {
        puts(c_filedata);
    }
    return 0;
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
    char *c_filedata;
    unsigned int c_filesize;

    int result = cpio_get_file(filepath, &c_filesize, &c_filedata);
    if (result == CPIO_TRAILER)
    {
        puts("exec: ");
        puts(filepath);
        puts(": No such file or directory\r\n");
        return -1;
    }
    else if (result == CPIO_ERROR)
    {
        puts("cpio parse error\r\n");
        return -1;
    }

    // thread_t *t = thread_create(c_filedata, filepath);
    // t->code = kmalloc(c_filesize);
    // t->datasize = c_filesize;
    // t->context.lr = (uint64_t)t->code; // set return address to program if function call completes
    // // copy file into code
    // memcpy(t->code, c_filedata, c_filesize);
    // eret to exception level 0
    // if (kernel_fork() == 0)
    // { // child process
    lock_interrupt();
        curr_thread->datasize = c_filesize;
        curr_thread->code = kmalloc(c_filesize);
        curr_thread->context.lr = (uint64_t)curr_thread->code; // set return address to program if function call completes
        strcpy(curr_thread->name, filepath);
        memcpy(curr_thread->code, c_filedata, c_filesize);
        DEBUG("c_filedata: 0x%x\r\n", c_filedata);
        DEBUG("child process, pid: %d\r\n", curr_thread->pid);
        DEBUG("curr_thread->code: 0x%x\r\n", curr_thread->code);

        asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
            "msr elr_el1, %1\n\t"   // When el0 -> el1, store return address for el1 -> el0
            "msr spsr_el1, xzr\n\t" // Enable interrupt in EL0 -> Used for thread scheduler
            "msr sp_el0, %2\n\t"    // el0 stack pointer for el1 process
            "mov sp, %3\n\t"        // sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
            ::"r"(&curr_thread->context),
            "r"(curr_thread->context.lr), "r"(curr_thread->context.sp), "r"(curr_thread->kernel_stack_base + KSTACK_SIZE));

        unlock_interrupt();

        asm("eret\n\t");
    // }
    // else
    // {
    //     while (1)
    //     {
    //         schedule();
    //         // dump_run_queue();
    //         // DEBUG("kshell while loop curr_thread->pid: %d\r\n", curr_thread->pid);
    //     }
    // }
    // return 0;
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