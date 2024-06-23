#include "io.h"
#include "fork.h"
#include "schedule.h"

extern void shell_loop();
extern void frame_init_with_reserve();
extern void memory_pool_init();
extern void mem_init();
extern void uart_init();
extern void timer_init();
extern void enable_irq();
extern void initramfs_callback(void*);
extern void rootfs_init();

#ifndef QEMU
extern void fdt_traverse(void (*callback)(void*));
#endif

static void multiple_init();

extern struct task_struct *current;

int main()
{
    multiple_init();
    enable_irq();

#ifndef QEMU
    fdt_traverse(initramfs_callback);
#endif
    frame_init_with_reserve();

    rootfs_init();

    printf("\nWelcome to Yuchang's Raspberry Pi 3!\n");

    // copy_process((unsigned long)(void*)&shell_loop, 0);
    copy_process(PF_KTHREAD, (unsigned long)(void*)&shell_loop, 0, 0);

    while(1){
        kill_zombies();
        schedule();
    }
    return 0;
}

static void multiple_init()
{
    mem_init();
    uart_init();
    memory_pool_init();
    timer_init();
    // rootfs_init();
}