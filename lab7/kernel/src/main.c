#include "uart.h"
#include "mailbox.h"
#include "shell.h"
#include "cpio.h"
#include "allocator.h"
#include "dtb_parser.h"
#include "exception.h"
#include "task.h"
#include "timer.h"
#include "allocator.h"
#include "schedule.h"
#include "syscall.h"
#include "vfs.h"
#include "utils.h"

int main()
{
    uart_init();
    fdt_traverse(initramfs_callback);
    build_file_arr();
    startup_allocate();
    task_heap_init();
    timer_heap_init();
    rootfs_init();
    sched_init();

    uart_puts("\n");
    enable_aux_interrupt();
    enable_interrupt();
    asm volatile(
        "mov x0, 1\n"
        "msr cntp_ctl_el0, x0\n"); // enable core0 timer

    simple_shell();

    return 0;
}
