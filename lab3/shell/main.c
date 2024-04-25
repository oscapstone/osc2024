#include "header/utils.h"
#include "header/uart.h"
#include "header/shell.h"
#include "header/cpio.h"
#include "header/dtb.h"
#include "header/uart.h"
#include "header/timer.h"
#include "header/task.h"
extern char *dtb_base;
int main(char *arg){
    register unsigned long long x21 asm("x21");
    // pass by x21 reg
    dtb_base = (char*)x21;

    // // print addresses
    // unsigned long el = 0;
	// asm volatile ("mrs %0, CurrentEL":"=r"(el));
	// uart_send_str("Current exception level: ");
	// uart_binary_to_hex(el>>2); // CurrentEL store el level at [3:2]
	// uart_send_str("\n");
	// asm volatile("mov %0, sp"::"r"(el));
	// uart_send_str("Current stack pointer address: ");
	// uart_binary_to_hex(el);
	// uart_send_str("\n");
    
    // fdt init
    fdt_traverse(initramfs_callback);
    uart_init();
    irqtask_list_init();
    timer_list_init();
    
    uart_send_str("\x1b[2J\x1b[H");

    // init interrupt
    // enable_timer();
    uart_interrupt_enable();
    asm volatile("msr DAIFClr, 0xf");
    core_timer_interrupt_enable();
    core_timer_interrupt_disable_alternative();
    
    

    char *s = "Type in `help` to get instruction menu!\r\n";
    uart_send_str(s);

    shell();
    return 0;
}