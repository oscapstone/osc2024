#include "dtb.h"
#include "exception.h"
#include "irqtask.h"
#include "memory.h"
#include "sched.h"
#include "shell.h"
#include "timer.h"
#include "uart1.h"
#include "vfs.h"

void main(char *arg)
{
    char input_buffer[CMD_MAX_LEN];

    dtb_ptr = PHYS_TO_VIRT(arg);
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs); // get initramfs location from dtb

    init_allocator();

    uart_init();
    irqtask_init_list();
    timer_list_init();

    init_thread_sched();

    // Register filesystems
    init_rootfs();

    uart_interrupt_enable();
    el1_interrupt_enable(); // enable interrupt in EL1 -> EL1
    core_timer_enable();

    uart_puts("\n");
    while (1) {
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts("# ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
}
