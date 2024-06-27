#include <bsp/mailbox.h>
#include <kernel/commands.h>
#include <kernel/io.h>

void _mailbox_command(int argc, char **argv) {
    puts("\nMailbox info :\n");
    unsigned int r = get_board_revision();
    puts("board revision : ");
    print_h(r);
    puts("\r\n");
    unsigned int base_addr, size;
    get_memory_info(&base_addr, &size);
    puts("ARM memory base address : ");
    print_h(base_addr);
    puts("\r\n");
    puts("ARM memory size : ");
    print_h(size);
    puts("\r\n");
}

command_t mailbox_command = {.name = "mailbox",
                                  .description = "print hardware info",
                                  .function = &_mailbox_command};
