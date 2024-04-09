#include "initramfs.h"
#include "mailbox.h"
#include "malloc.h"
#include "shell.h"
#include "string.h"
#include "uart.h"

void shell_start()
{
    char *line = NULL;
    while (1) {
        uart_puts("# ");
        getline(&line, MAX_GETLINE_LEN);    // FIXME: too many malloc without free?
        do_cmd(line);
    }
}

// Define Shell Commands
static cmt_t command_funcs[] = {
    cmd_help,
    cmd_hello,
    cmd_reboot,
    get_board_revision,
    get_arm_memory,
    list_initramfs,
    cat_initramfs,
    cmd_execute
};
static char* commands[] = {
    "help",
    "hello",
    "reboot",
    "board",
    "arm",
    "ls",
    "cat",
    "exec"
};
static char* command_descriptions[] = {
    "print this help menu",
    "print Hello World!",
    "reboot the device",
    "print board info",
    "print arm memory info",
    "list initramfs",
    "cat a file",
    "execute a program in userspace (EL0)"
};

void do_cmd(const char* line)
{
    int size = sizeof(command_funcs) / sizeof(command_funcs[0]);
    for (int i = 0; i < size; i++) {
        if (strcmp(line, commands[i]) == 0) {
            command_funcs[i]();
            return;
        }
    }
    cmd_default();
    return;
}

void cmd_help()
{
    int size = sizeof(command_descriptions) / sizeof(command_descriptions[0]);
    for (int i = 0; i < size; i++) {
        uart_puts(commands[i]);
        uart_puts("\t: ");
        uart_puts(command_descriptions[i]);
        uart_puts("\n");
    }
}

void cmd_hello()
{
    uart_puts("Hello World!\n");
}

void cmd_reboot()
{
    uart_puts("Reboot!\n");

    unsigned int r;
    r = *PM_RSTS;
    r &= ~0xFFFFFAAA;
    *PM_RSTS = PM_WDOG_MAGIC | r;
    *PM_WDOG = PM_WDOG_MAGIC | 10;
    *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
}

void cmd_default()
{
    uart_puts("command not found\n");
}

/**
 * Execute binary executable from initramfs in userspace (EL0).
*/
void cmd_execute(void)
{
    char *filename = NULL;
    uart_puts("Filename: ");
    getline(&filename, 0x20);

    // Find file from initramfs
    cpio_meta_t *f = find_initramfs(filename);
    if (f == NULL) {
        return;
    }

    memcpy(&__userspace_start, f->content, strlen_new(f->content));

    asm("mov x0, 0x3c0");
    asm("msr spsr_el1, x0");                                // state
    asm("msr elr_el1, %0" : : "r" (&__userspace_start));    // return address
    asm("msr sp_el0, %0" : : "r" (&__userspace_end));       // stack pointer
    asm("eret");
}
