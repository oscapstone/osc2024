#include "mini_uart.h"
#include "shell.h"
#include "string.h"
#include "heap.h"
#include "devicetree.h"
#include "initrd.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "vm.h"

extern int kernel_start;
extern int kernel_end;

extern void set_exception_vector_table();

void main(void* dtb_ptr){
    uart_init();
    set_exception_vector_table();
    fdt_traverse(dtb_ptr, initramfs_callback);
    heap_init();
    enable_irq();
    core_timer_enable();
    buddy_init();

    reserve((void *)phys_to_virt(0x0), (void *)phys_to_virt((void*)0x3fff)); // spin table && PGD && PUD && PMD
    reserve((void *)((char*)&kernel_start-0x20000), (void *)((char*)&kernel_start-0x0001)); // stack
    reserve((void *)&kernel_start, (void *)&kernel_end); // kernel
    reserve((void *)phys_to_virt(get_initrd_start()), (void *)phys_to_virt(get_initrd_end())); // initrd
    reserve((void *)phys_to_virt(dtb_ptr), (void *)phys_to_virt(get_dtb_end())); // dtb
    reserve((void *)&bss_end, heap_end); // heap

    kmem_cache_init();
    thread_init();

    shell_exec();
}