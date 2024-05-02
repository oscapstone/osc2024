#include "mini_uart.h"
#include "shell.h"
#include "string.h"
#include "heap.h"
#include "devicetree.h"
#include "initrd.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"

// static char user_input[BUFFER_SIZE];
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
  
    reserve((void *)0x0, (void *)0x1000); // spin table
    reserve((void *)((char*)&kernel_start-0x20000), (void *)((char*)&kernel_start-0x0001)); // stack
    reserve((void *)&kernel_start, (void *)&kernel_end); // kernel
    reserve(get_initrd_start(), get_initrd_end()); // initrd
    reserve(dtb_ptr, get_dtb_end()); // dtb
    reserve((void *)&bss_end, heap_end); // heap

    kmem_cache_init();

    void *a = alloc_pages(2);
    void *b = alloc_pages(2);
    free_pages(a);
    free_pages(b);
    void *d = kmalloc(40);
    void *e = kmalloc(64);
    kfree(d);
    kfree(e);
    

    shell_exec();
}