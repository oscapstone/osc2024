#include <kernel/bsp_port/ramfs.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <lib/stddef.h>
#include <lib/string.h>
#include <kernel/timer.h>
#include <kernel/sched.h>
#include <kernel/commands.h>

void _exec_command(int argc, char **argv) {
    if (argc < 2) {
        print_string("\nUsage: exec <filename>");
        return;
    }

    char *file_name = argv[1];
    unsigned long file_size = ramfs_get_file_size(file_name);
    char *file_contents = kmalloc(file_size);
    ramfs_get_file_contents(file_name, file_contents);

    if (file_contents == NULL) {
        print_string("[ERROR] File not found: ");
        print_string(file_name);
        print_string("\n");
        return;
    }
    print_string("\n[INFO] Executing file: ");
    print_string(file_name);
    print_string("\n");

    char *user_program = kmalloc(file_size);
    memcpy(user_program, file_contents, file_size);

    char buf[100];
    sprintf(buf, "User program pointer: %x\n", user_program);
    print_string(buf);

    preempt_disable();
    get_current()->state = TASK_STOPPED;
    print_string("Enabling user to physical timer\n");
    enable_user_to_physical_timer();
    print_string("Copying process\n");
    copy_process(PF_KTHREAD, (unsigned long)&move_to_user_mode, (unsigned long)user_program, 0);
    preempt_enable();
}

command_t exec_command = {.name = "exec",
                           .description = "execute a user program",
                           .function = &_exec_command};
