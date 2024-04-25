#include "dtb.h"
#include "uart.h"
#include "irq.h"
#include "list.h"
#include "timer.h"
#include "mm.h"

extern char *dtb_base;


void main(char *arg)
{
    //register unsigned long long x21 asm("x21");
    // pass by x21 reg
    dtb_base = arg;
    // init list
    INIT_LIST_HEAD(&timer_event_list);
    INIT_LIST_HEAD(&task_list);
    init_idx();
   

    //uart_puts("t1\n");
    

    // turn on interrupt
    core_timer_enable();
    //uart_puts("t2\n");
    core_timer_interrupt_disable_alternative();
    //uart_puts("t3\n");
    core_timer_interrupt_enable();
   
    uart_init();
      //uart_puts("t4\n");
    enable_mini_uart_interrupt();
    //uart_puts("t5\n");
    enable_interrupt();
   // uart_puts("t6\n");
    // turn off to enable printf
    // 控制在EL1和EL0時 access advanced SIMD 或 floating point時會不會產生exception

    
    // get devicetree
    fdt_traverse(initramfs_callback);
  
    memory_init();
    uart_puts("Lab4 :\n");

    shell();
}