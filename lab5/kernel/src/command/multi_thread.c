#include <bsp/asm/mailbox.h>
#include <kernel/bsp_port/syscall.h>
#include <kernel/commands.h>
#include <kernel/io.h>
#include <kernel/sched.h>
#include <lib/utils.h>

void tt() {
     for (int i = 0; i < 3; i++)
        kthread_create(thread_test);
    idle();
}

command_t test_thread_command = {
    .name = "thread",
    .description = "demo thread user program",
    .function = &tt,
};

void entry() {
    kthread_create(run_fork_test);
    idle();
}

command_t test_multi_thread_command = {
    .name = "multi_thread",
    .description = "demo multi-thread user program",
    .function = &entry,
};
