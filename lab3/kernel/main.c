#include "bsp/irq.h"
#include "bsp/ramfs.h"
#include "bsp/uart.h"
#include "console.h"
#include "device_tree.h"
#include "io.h"
#include "kernel/timer.h"
#include "memory.h"
#include "task_queue.h"

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

    struct Console *console = console_create();
    register_all_commands(console);

    uart_puts("\nWelcome to YJack0000's shell\n");
    while (1) {
        run_console(console);
    }

    return 0;
}
