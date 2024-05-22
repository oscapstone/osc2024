#include "dtb.h"
#include "uart.h"
#include "irq.h"
#include "cpio.h"
#include "list.h"
#include "timer.h"
#include "task.h"
#include "sched.h"
#include "mm.h"
#include "mbox.h"


extern char *dtb_base;

void foo(){
    for(int i = 0; i < 10; ++i) {
        uart_puts("tets\n");
        uart_printf("Thread id: %d %d\n", curr_thread->pid, i);
        for(int j = 0; j < 1000000; ++j);
        schedule();
    }
}

void kernel_main() {
    // ...
    // boot setup
    // ...
    for(int i = 0; i < 3; ++i) { // N should > 2
        thread_create(foo);
        uart_printf("Thread id: %d %d\n", curr_thread->pid, i);
    }
    uart_puts("idle\n");
    idle();
}
void fork_test(){
    uart_printf("\nFork Test, pid %d\n", curr_thread->pid);
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        uart_printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", curr_thread->pid, cnt, &cnt, cur_sp);
        ++cnt;

        if ((ret = fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            uart_printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", curr_thread->pid, cnt, &cnt, cur_sp);
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                uart_printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", curr_thread->pid, cnt, &cnt, cur_sp);
                for(int z = 0; z < 10000000; ++z);
                ++cnt;
            }
        }
        exit();
    }
    else {
        uart_printf("parent here, pid %d, child %d\n",curr_thread->pid, ret);
    }
}

void main(char *arg)
{

    dtb_base = arg;
    // init list
    init_idx();
    uart_init();
    INIT_LIST_HEAD(&task_list);
    fdt_traverse(initramfs_callback);
    memory_init();
    timer_list_init();
   
    enable_interrupt();
    enable_mini_uart_interrupt();
   
    // lab5 init thread 
    init_thread_sched();
   
    core_timer_enable();
    // test program will access cpu timer register
    cpu_timer_enable();
  

    uart_printf("Lab5 :\n");
    //fork_test();
    //kernel_main();
    execfile("syscall.img");
    //shell();
}
