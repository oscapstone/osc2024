#include "mini_uart.h"
#include "shell.h"
#include "reboot.h"
#include "timer.h"
#include "dtb.h"
#include "cpio.h"
#include "sysregs.h"
char input_buffer[CMD_MAX_LEN];

extern void el0_enter(void);
extern char *_dtb;
void *CPIO_DEFAULT_PLACE = (void *)(unsigned long)0x8000000;
int entered_user_program = 0;

struct CLI_CMDS cmd_list[11] = {
    {.command = "cat", .help = "concatenate files and print on the standard output", .func = shell_cat},
    {.command = "dtb", .help = "show device tree", .func = shell_dtb},
    {.command = "exec", .help = "execute a command, replacing current image with a new image", .func = shell_exec},
    {.command = "hello", .help = "print Hello World!", .func = shell_hello_world},
    {.command = "help", .help = "print all available commands", .func = shell_help},
    {.command = "info", .help = "get device information via mailbox", .func = shell_info},
    {.command = "malloc", .help = "test kmalloc", .func = shell_malloc},
    {.command = "ls", .help = "list directory contents", .func = shell_ls},
    {.command = "setTimeout", .help = "setTimeout [MESSAGE] [SECONDS]", .func = shell_setTimeout},
    {.command = "2sTimer", .help = "set core timer interrupt every 2 second", .func = shell_2sTimer},
    {.command = "timer", .help = "turn on timer", .func = shell_timer},
    {.command = "reboot", .help = "reboot the device", .func = shell_reboot},
};

int shell_cmd_strcmp(const char *p1, const char *p2)
{
    const unsigned char *s1 = (const unsigned char *)p1;
    const unsigned char *s2 = (const unsigned char *)p2;
    unsigned char c1, c2;

    do
    {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);
    return c1 - c2;
}

void shell_banner(void)
{
    uart_puts("\n==================Now is 2024==================\n");
    uart_puts("||    Here comes the OSC 2024 Lab3 kernel      ||\n");
    uart_puts("================================================\n");
}

void shell_cmd_read(char *buffer)
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

void shell_cmd_exe(char *buffer)
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
        if (shell_cmd_strcmp(cmd, cmd_list[i].command) == 0)
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
                if (*argc >= 10) // do not exceed 10
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

void shell_setTimeout(int argc, char **argv)
{
    char *msg, *sec;
    if (argc == 2)
    {
        msg = argv[0];
        sec = argv[1];
    }
    else
    {
        puts("Incorrect number of parameters\r\n");
        return -1;
    }
    add_timer(puts, atoi(sec), msg);
    return 0;
}
void shell_2sTimer(){

    // add_timer(The2sTimer, 1, "2sAlert");
    add_timer(core_timer_intr_handler, 2, "2sAlert");
}

void shell_dtb(int argc, char **argv)
{
    // traverse_device_tree(dtb_ptr, dtb_callback_show_tree);
}

void shell_timer()
{
    timer_init();
}
void shell_into_EL0()
{
    // put_currentEL();
    if (entered_user_program == 0)
    {
        entered_user_program = 1;
        // el0_enter();
        /* code */
    }
    else
    {
        put_currentEL();
        uart_puts("Already in User Mode \r\n");
    }
};

void shell_clear(char *buffer, int len)
{
    for (int i = 0; i < len; i++)
    {
        buffer[i] = '\0';
    }
}

void shell()
{
    shell_clear(input_buffer, CMD_MAX_LEN);
    uart_puts("ヽ(▽ﾟ▽ﾟ)ノ >> \t");
    shell_cmd_read(input_buffer);
    shell_cmd_exe(input_buffer);
}

void shell_hello_world()
{
    uart_puts("hello world!\n");
}

void shell_help()
{
    for (int i = 0; i < CLI_MAX_CMD; i++)
    {
        puts(cmd_list[i].command);
        puts("\t\t\t: ");
        puts(cmd_list[i].help);
        puts("\r\n");
    }
}

void shell_info()
{
    unsigned int board_revision;
    get_board_revision(&board_revision);
    uart_puts("Board revision is : 0x");
    uart_hex(board_revision);
    uart_puts("\n");

    unsigned int arm_mem_base_addr;
    unsigned int arm_mem_size;

    get_arm_memory_info(&arm_mem_base_addr, &arm_mem_size);
    uart_puts("ARM memory base address in bytes : 0x");
    uart_hex(arm_mem_base_addr);
    uart_puts("\n");
    uart_puts("ARM memory size in bytes : 0x");
    uart_hex(arm_mem_size);
    uart_puts("\n");

    uart_puts("\n");
    uart_puts("This is a simple shell for raspi3.\n");
    uart_puts("type help for more information\n");
}

void shell_reboot()
{
    reset(196608);
}

void shell_cancel_reboot()
{
    cancel_reset();
    uart_puts("reboot canceled. \n");
}

void shell_ls(int argc, char **argv)
{
    char *workdir;
    char *c_filepath;
    char *c_filedata;
    unsigned int c_filesize;
    // CPIO_DEFAULT_PLACE = (void *)(unsigned long)0x8000000;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;
    if (argc == 0)
    {
        workdir = ".";
    }
    else
    {
        workdir = argv[0];
    }

    while (header_ptr != 0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if (error)
        {
            uart_puts("cpio parse error");
            break;
        }

        // if this is not TRAILER!!! (last of file)
        if (header_ptr != 0)
        {
            uart_puts(c_filepath);
            uart_puts("\n");
        }
    }
}

void shell_cat(int argc, char **argv)
{
    char *filepath;
    char *c_filepath;
    char *c_filedata;
    unsigned int c_filesize;
    CPIO_DEFAULT_PLACE = (void *)(unsigned long)0x8000000;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

    if (argc == 1)
    {
        filepath = argv[0];
    }
    else
    {
        puts("Incorrect number of parameters\r\n");
        return -1;
    }
    while (header_ptr != 0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        // if parse header error
        if (error)
        {
            uart_puts("cpio parse error");
            break;
        }

        if (shell_cmd_strcmp(c_filepath, filepath) == 0)
        {
            uart_puts(c_filedata);
            break;
        }

        // if this is TRAILER!!! (last of file)
        if (header_ptr == 0)
        {
            uart_puts("cat: ");
            uart_puts(c_filepath);
            uart_puts(" No such file or directory\n");
        }
    }
}

void shell_exec(int argc, char **argv)
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
    char *c_filepath;
    char *c_filedata;
    unsigned int c_filesize;
    CPIO_DEFAULT_PLACE = (void *)(unsigned long)0x8000000;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

    while (header_ptr != 0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        // if parse header error
        if (error)
        {
            uart_puts("cpio parse error");
            break;
        }

        if (shell_cmd_strcmp(c_filepath, filepath) == 0)
        {
            // exec c_filedata
            char *ustack = malloc(0x1000);
            asm volatile("msr elr_el1, %0\n\t"   // elr_el1: Set the address to return to: c_filedata
                        " mov x3, 0x3c0\n\t"
                         "msr spsr_el1, x3\n\t" // enable interrupt (PSTATE.DAIF) -> spsr_el1[9:6]=4b0. In Basic#1 sample, EL1 interrupt is disabled.
                         "msr sp_el0, %1\n\t"    // user program stack pointer set to new stack.
                         "eret\n\t"              // Perform exception return. EL1 -> EL0
                         :
                         : "r"(c_filedata), "r"(ustack + 0x1000));
            break;
        }

        // if this is TRAILER!!! (last of file)
        if (header_ptr == 0)
        {
            uart_puts("cat: ");
            uart_puts(filepath);
            uart_puts(": No such file or directory\n");
        }
    }
}

char *strcpy1(char *dest, const char *src)
{
    char *d = dest;
    const char *s = src;
    while ((*d++ = *s++))
        ;

    return dest;
}
void shell_malloc()
{
    // test malloc
    // char *test1 = malloc(0x22);
    // strcpy1(test1, "test malloc1 \n");
    // uart_putss(test1);
    // uart_hex(test1);
    // uart_puts("\n");

    // char *test2 = malloc(0x10);
    // strcpy1(test2, "test malloc2 \n");
    // uart_putss(test2);
    // uart_hex(test2);
    // uart_puts("\n");

    // char *test3 = malloc(0x10);
    // strcpy1(test3, "test malloc3 \n");
    // uart_putss(test3);
    // uart_hex(test3);
    // uart_puts("\n");
}
