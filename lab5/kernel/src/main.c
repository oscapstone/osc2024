#include <kernel/bsp_port/irq.h>
#include <kernel/bsp_port/ramfs.h>
#include <kernel/bsp_port/uart.h>
#include <kernel/console.h>
#include <kernel/device_tree.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <kernel/timer.h>

void print_boot_timeout(int delay) {
    print_string("\nBoot Timeout: ");
    print_d((const int)delay);
    print_string("s\n");
}

void aa() {
    while(1) {
        for(int i = 0; i < 100000000; i++);
        print_string("--------------aa\n");
    }
    exit_process();
}

int main() {
    // lab1
    uart_init();

    // lab2
    init_memory();
    fdt_traverse(init_ramfs_callback);

    // lab3
    enable_irq();
    // set_timeout((void *)print_boot_timeout, (void *)4, 4);
    core_timer_enable();

    // lab4
    init_kmalloc();

    // lab5
    sched_init();

    struct Console *console = console_create();
    register_all_commands(console);

    uart_puts("\nWelcome to YJack0000's shell\n");

    copy_process(PF_KTHREAD, (unsigned long)(void *)&run_console,
                 (unsigned long)console, 0);
    // print_string("start aa\n");
    // copy_process(PF_KTHREAD, (unsigned long)(void *)&aa, 0, 0);
    // print_string("start bb\n");
    // copy_process(PF_KTHREAD, (unsigned long)(void *)&bb, 0, 0);

    // run_console(console);

    // root_task();

    return 0;
}
