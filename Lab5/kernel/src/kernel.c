#include "cpio.h"
#include "dtb.h"
#include "fork.h"
#include "int.h"
#include "irq.h"
#include "logo.h"
#include "memory.h"
#include "mini_uart.h"
#include "page_alloc.h"
#include "sched.h"
#include "shell.h"
#include "slab.h"
#include "string.h"
#include "thread.h"
#include "timer.h"
#include "utils.h"

static void idle(void)
{
    while (1) {
        kill_zombies();
        schedule();
    }
}

void kernel_main(uintptr_t dtb_ptr)
{
    uart_init();
    uart_printf("Welcome to Raspberry Pi 3B+\n");
    send_logo();

    uart_printf("Current exception level: %d\n", get_el());

    irq_vector_init();

    if (!mem_init((uintptr_t)dtb_ptr))
        goto inf_loop;

    buddy_init();

    kmem_cache_init();

    if (!irq_init())
        goto inf_loop;

    if (!timer_init())
        goto inf_loop;

    if (!sched_init())
        goto inf_loop;

    enable_irq();

    // test_thread();

    copy_process(PF_KTHREAD, shell, NULL);
    // cpio_exec("syscall.img");

    // copy_process(PF_KTHREAD, kernel_process, fork_test);
    // copy_process(PF_KTHREAD, kernel_process, exec_test);

    idle();

inf_loop:
    while (1)
        ;
}
