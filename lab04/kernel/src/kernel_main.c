#include "mini_uart.h"
#include "shell.h"
#include "devicetree.h"
#include "io.h"
#include "string.h"
#include "type.h"
#include "cpio.h"
#include "irq.h"
#include "timer.h"
#include "alloc.h"

static void multiple_init();

int main()
{
    multiple_init();
    // add_timer((void*)print_time_handler, 0, 2);
    // core_timer_enable();
    enable_irq();

#ifndef QEMU
    fdt_traverse(initramfs_callback);
#endif

    printf("\nWelcome to Yuchang's Raspberry Pi 3!\n");

    while(1)
    {
        shell();
    }
    return 0;
}

static void multiple_init()
{
    time_head_init();
    mem_init();
    uart_init();
    task_head_init();
    uart_buff_init();
    frame_init();
}