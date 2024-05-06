#include <kernel/bsp_port/irq.h>
#include <kernel/bsp_port/ramfs.h>
#include <kernel/bsp_port/uart.h>
#include <kernel/console.h>
#include <kernel/device_tree.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/task_queue.h>
#include <kernel/timer.h>

void print_boot_timeout(int delay) {
    print_string("\nBoot Timeout: ");
    print_d((const int)delay);
    print_string("s\n");
}

int main() {
    // lab1
    uart_init();

    // lab2
    init_memory();
    fdt_traverse(init_ramfs_callback);

    // lab3
    el1_enable_interrupt();
    // set_timeout((void *)print_boot_timeout, (void *)4, 4);
    core_timer_enable();
    init_task_queue();

    // lab4
    init_kmalloc();

    struct Console *console = console_create();
    register_all_commands(console);

    uart_puts("\nWelcome to YJack0000's shell\n");
    while (1) {
        run_console(console);
    }

    return 0;
}
