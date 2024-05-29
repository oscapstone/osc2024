#include <kernel/bsp_port/syscall.h>
#include <kernel/commands.h>
#include <kernel/io.h>
#include <kernel/sched.h>
#include <lib/utils.h>

#define delay(x) \
    for (int i = 0; i < x; i++) asm volatile("nop");

void user_foo() {
    char buffer[256];  // Buffer for formatted string

    // sprintf(buffer, "\r\nFork Test, pid: %d", getpid());
    // print_string(buffer);
    // print_string("\r\nFork Test, pid: ");
    // print_d(get_current()->pid);
    print_string("\r\nFork Test, pid: ");
    print_d(getpid());
    print_string("\n");
    uartwrite("UART Write Test\n", 16);
    uint32_t cnt = 1, ret = 0;

    if ((ret = fork()) == 0) {  // pid == 0 => child
        print_string("\r\n===== Child Process =====");
        unsigned long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));

        sprintf(buffer, "\r\nfirst child pid: %d, cnt: %d, ptr: %lx, sp: %lx",
                getpid(), cnt, (unsigned long)&cnt, cur_sp);
        print_string(buffer);
        ++cnt;

        if ((ret = fork()) != 0) {
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            sprintf(buffer,
                    "\r\nfirst child pid: %d, cnt: %d, ptr: %lx, sp: %lx",
                    getpid(), cnt, (unsigned long)&cnt, cur_sp);
            print_string(buffer);
        } else {
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                sprintf(buffer,
                        "\r\nsecond child pid: %d, cnt: %d, ptr: %lx, sp: %lx",
                        getpid(), cnt, (unsigned long)&cnt, cur_sp);
                print_string(buffer);
                delay(1000000);  // Delay in microseconds
                ++cnt;
            }
        }
        exit(0);
    } else {  // pid > 0 => parent
        print_string("\r\n===== Parent Process =====");
        sprintf(buffer, "\r\nParent Process, pid: %d, child pid: %d", getpid(),
                ret);
        print_string(buffer);

        unsigned long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        sprintf(buffer, " cnt: %d, ptr: %lx, sp: %lx", cnt, (unsigned long)&cnt,
                cur_sp);
        print_string(buffer);
        exit(0);
    }
}

static void entry() {
    copy_process(PF_KTHREAD, (unsigned long)&move_to_user_mode, (unsigned long)&user_foo, 0);
    // int err = move_to_user_mode((unsigned long)&user_foo);
    // if (err < 0) {
    //     print_string("Error in move_to_user_mode\n");
    // }
}

command_t test_multi_thread_command = {
    .name = "multi_thread",
    .description = "demo multi-thread user program",
    .function = &entry,
};
