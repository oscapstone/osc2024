#ifndef _ALL_H
#define _ALL_H

struct Command {
    char name[16];
    char description[64];
    void (*function)(int argc, char **argv);
};

extern struct Command hello_command;
extern struct Command mailbox_command;
extern struct Command reboot_command;
extern struct Command ls_command;
extern struct Command cat_command;
extern struct Command test_malloc_command;
extern struct Command async_io_demo_command;
extern struct Command exec_command;
extern struct Command set_timeout_command;
extern struct Command test_kmalloc_command;
extern struct Command test_kfree_command;
extern struct Command test_multi_thread_command;

#endif
