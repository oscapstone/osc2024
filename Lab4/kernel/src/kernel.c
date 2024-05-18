#include "cpio.h"
#include "dtb.h"
#include "int.h"
#include "irq.h"
#include "logo.h"
#include "memory.h"
#include "mini_uart.h"
#include "page_alloc.h"
#include "shell.h"
#include "slab.h"
#include "string.h"
#include "timer.h"
#include "utils.h"

void kernel_main(uintptr_t dtb_ptr)
{
    uart_init();
    uart_printf("Welcome to Raspberry Pi 3B+\n");
    send_logo();

    uart_printf("Current exception level: %d\n", get_el());

    irq_vector_init();

    enable_irq();

    if (!mem_init((uintptr_t)dtb_ptr))
        goto inf_loop;

    buddy_init();

    kmem_cache_init();

    slabinfo();

    if (irq_init())
        goto inf_loop;

    if (timer_init())
        goto inf_loop;

    slabinfo();

    struct kmem_cache* fuck = kmem_cache_create("fuck", 10, -1);
    uart_printf("fuck = 0x%x\n", (uintptr_t)fuck);
    slabinfo();
    void* ptr = kmem_cache_alloc(fuck);
    uart_printf("ptr = 0x%x\n", (uintptr_t)ptr);
    slabinfo();
    kmem_cache_free(fuck, ptr);
    slabinfo();
    kmem_cache_destroy(fuck);
    slabinfo();


    void* ptr1 = kmalloc(10 * sizeof(int));
    kfree(ptr1);

    void* ptr2 = kmalloc(3 * 1024);
    kfree(ptr2);

    shell();

inf_loop:
    while (1)
        ;
}
