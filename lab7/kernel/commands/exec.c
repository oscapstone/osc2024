#include <bsp/ramfs.h>
#include <kernel/commands.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <kernel/timer.h>
#include <lib/stddef.h>
#include <lib/string.h>

void _exec_command(int argc, char **argv) {
    if (argc < 2) {
        puts("\nUsage: exec <filename>");
        return;
    }

    char *file_name = argv[1];
    unsigned long file_size = ramfs_get_file_size(file_name);
    char *file_contents = kmalloc(file_size);
    ramfs_get_file_contents(file_name, file_contents);

    if (file_contents == NULL) {
        puts("[ERROR] File not found: ");
        puts(file_name);
        puts("\n");
        return;
    }
    puts("\n[INFO] Executing file: ");
    puts(file_name);
    puts("\n");

    char *user_program = kmalloc(file_size);
    memcpy(user_program, file_contents, file_size);
    enable_user_to_physical_timer();
    struct task_struct *task = kthread_create((void *)user_program);
    asm volatile("msr tpidr_el1, %0;" ::"r"(task));
    asm volatile("msr spsr_el1, %0" ::"r"(0x0));
    asm volatile("msr elr_el1, %0" ::"r"(task->context.lr));
    asm volatile("msr sp_el0, %0" ::"r"(task->user_stack + STACK_SIZE));
    asm volatile("mov sp, %0" ::"r"(task->stack + STACK_SIZE));
    asm volatile("eret");

    // char buf[100];
    // sprintf(buf, "User program pointer: %x\n", user_program);
    // puts(buf);

    // preempt_disable();
    // get_current()->state = TASK_STOPPED;
    // puts("Enabling user to physical timer\n");
    // enable_user_to_physical_timer();
    // puts("Copying process\n");
    // copy_process(PF_KTHREAD, (unsigned long)&move_to_user_mode, (unsigned
    // long)user_program, 0); preempt_enable();
}

command_t exec_command = {.name = "exec",
                          .description = "execute a user program",
                          .function = &_exec_command};
