#include "../bsp/mailbox.h"

#include "../io.h"
#include "all.h"

void _mailbox_command(int argc, char **argv) {
    print_string("\nMailbox info :\n");
    unsigned int r = get_board_revision();
    print_string("board revision : ");
    print_h(r);
    print_string("\r\n");
    unsigned int base_addr, size;
    get_memory_info(&base_addr, &size);
    print_string("ARM memory base address : ");
    print_h(base_addr);
    print_string("\r\n");
    print_string("ARM memory size : ");
    print_h(size);
    print_string("\r\n");
}

struct Command mailbox_command = {.name = "mailbox",
                                  .description = "print hardware info",
                                  .function = &_mailbox_command};
