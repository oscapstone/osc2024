#include "sched.h"
#include "uart.h"
#include "exec.h"
#include "syscall.h"
#include "delays.h"
#include "exception.h"
#include "interrupt.h"
#include "timer.h"
#include "mm.h"
#include "memblock.h"

/* Only in demo.c, for delay usage. */
void delay()
{
    long count = 10000000000; // 10000000000 in qemu can see pid 3; 100000000 in raspi 3 can see pid 3.
    while (count--) {
        asm volatile("nop");
    };
}

/* osc2024, demo system calls. */
void fork_test(void)
{
    printf("\nFork Test, pid %d\n", get_taskid());
    int cnt = 1, ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        printf("first child, pid %d, cnt: %d, ptr: %x, sp: %x\n", get_taskid(), cnt, &cnt, cur_sp);
        ++cnt;

        if ((ret = fork()) != 0) {
            delay();
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            printf("first child, pid: %d, cnt: %d, ptr: %x, sp: %x\n", get_taskid(), cnt, &cnt, cur_sp);
        } else {
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                printf("second child, pid: %d, cnt: %d, ptr: %x, sp: %x\n", get_taskid(), cnt, &cnt, cur_sp);
                delay();
                ++cnt;
            }
        }
        printf("child %d exit.\n", get_taskid());
        exit(0);
    } else {
        printf("parent here, pid %d, child %d\n", get_taskid(), ret);
        delay();
        printf("parent exit.\n");
        exit(0);
    }
}

/* Lab 3 Basic Exercise 3: Asynchronous UART */
void demo_async_uart()
{
    char string[64];
    int len;

    uart_async_init(); // Initialize the Asynchronous UART
    uart_async_puts("Demo lab 3: Asynchronous UART\n"); // Put the string to write_buffer
    wait_cycles(150);
    while (1) {
        len = uart_async_gets(string); // Get all data in read_buffer
        wait_cycles(150);
        if (len > 0) {
            uart_hex(len); // output the length of the string
            uart_async_puts("\n");
            uart_async_puts(string); // output the string in read_buffer
            uart_async_puts("\n");
        }
        wait_cycles(10000);
    };
}

/* Lab3 Advanced 2: bottom half irq */
void demo_bh_irq(void)
{
    enable_interrupt(); /* In this demo, we are in el1. So enable interrupt in el1 first. */
    core_timer_enable();
    uart_async_init();
    uart_puts("Demo Lab3 Advanced 2: bottom half irq\n");
    while (1) {
        do_tasklet();
        // wait_cycles(1000000000);
    }
}

/* Lab4: Demo buddy system and slab */
void demo_memory_allocator(void)
{
    char *addr[10];
    int tmp = 6;

    /* Print the reservation information (printf has bugs, so print in qemu for now) */
    print_memblock_info();

    get_buddy_info();
    uart_puts("==Get order 2 page for 6 times==\n\n");
    for (int i = 0; i < tmp; i++) {
        addr[i] = (char *) kmalloc(4096 << 2);
        printf("addr: %x\n\n", addr[i]);
    }
    get_buddy_info();
    uart_puts("Free order 2 page for 6 times\n");
    for (int i = 0; i < tmp; i++)
        kfree(addr[i]);
    get_buddy_info();

    addr[0] = (char *) kmalloc(8);
    printf("Get 8 bytes memory addr: %x\n", addr[0]);

    addr[0] = (char *) kmalloc(8);
    printf("Get 8 bytes memory addr: %x\n", addr[0]);

    addr[1] = (char *) kmalloc(16);
    printf("Get 16 bytes memory addr: %x\n", addr[1]);

    addr[2] = (char *) kmalloc(16);
    printf("Get 16 bytes memory addr: %x\n", addr[2]);

    uart_puts("Free the previous 16 bytes\n");
    kfree(addr[1]);

    addr[1] = (char *) kmalloc(16);
    printf("Get 16 bytes memory addr: %x\n", addr[1]);

    addr[3] = (char *) kmalloc(16);
    printf("Get 16 bytes memory addr: %x\n", addr[3]);
}