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

int main() {
    // lab1
    uart_init();
    init_console();

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
    // sched_init();
    kthread_init();

    print_string("\nWelcome to YJack0000's shell\n");

    // set_timeout((void*)print_boot_timeout, 0, 1);
    // kthread_create((void *)run_console);
    run_console();

    return 0;
}
