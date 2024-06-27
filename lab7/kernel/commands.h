#ifndef _COMMAND_H_
#define _COMMAND_H_

typedef struct command {
    char name[16];
    char description[64];
    void (*function)(int argc, char **argv);
} command_t;

extern command_t hello_command;
extern command_t mailbox_command;
extern command_t reboot_command;
extern command_t ls_command;
extern command_t cat_command;
extern command_t test_malloc_command;
extern command_t async_io_demo_command;
extern command_t exec_command;
extern command_t set_timeout_command;
extern command_t test_kmalloc_command;
extern command_t test_kfree_command;
extern command_t test_multi_thread_command;
extern command_t test_thread_command;

extern command_t rootfs_ls_command;
extern command_t rootfs_cat_command;
extern command_t rootfs_mkdir_command;
extern command_t rootfs_cd_command;
extern command_t rootfs_cwd_command;
extern command_t rootfs_touch_command;

#endif
