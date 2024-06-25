#include <stdint.h>

#include "exception.h"
#include "mini_uart.h"
#include "peripherals/irq.h"
#include "timer.h"
#include "tasklist.h"
#include "syscall.h"
#include "alloc.h"
#include "thread.h"
#include "reboot.h"
#include "signal.h"
#include "fs_vfs.h"
#include "c_utils.h"

extern timer_t *timer_head;

void el1_interrupt_enable(){
    asm volatile ("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable(){
    asm volatile ("msr daifset, 0xf"); // mask all DAIF
}

void exception_handler_c() {
    uart_send_string("Exception Occurs!\n");

    //read spsr_el1
	unsigned long long spsr_el1 = 0;
	asm volatile("mrs %0, spsr_el1":"=r"(spsr_el1));
	uart_send_string("spsr_el1: ");
	uart_hex(spsr_el1);
	uart_send_string("\n");

	//read elr_el1
	unsigned long long elr_el1 = 0;
	asm volatile("mrs %0, elr_el1":"=r"(elr_el1));
	uart_send_string("elr_el1: ");
	uart_hex(elr_el1);
	uart_send_string("\n");
	
	//esr_el1
	unsigned long long esr_el1 = 0;
	asm volatile("mrs %0, esr_el1":"=r"(esr_el1));
    uart_send_string("esr_el1: ");
	uart_hex(esr_el1);
	uart_send_string("\n");

	//ec
	unsigned ec = (esr_el1 >> 26) & 0x3F; //0x3F = 0b111111(6)
	uart_send_string("ec: ");
	uart_hex(ec);
	uart_send_string("\n");
    // while(1);
    reset(1);
}

void user_irq_exception_handler_c() {
    // uart_send_string("User IRQ Exception Occurs!\n");
    el1_interrupt_disable();
    unsigned int irq = *IRQ_PENDING_1;
    unsigned int interrupt_source = *CORE0_INTERRUPT_SOURCE;

    if((irq & IRQ_PENDING_1_AUX_INT) && (interrupt_source & INTERRUPT_SOURCE_GPU)){
        // uart_send_string("UART interrupt\n");
        uart_irq_handler();
    }
    if(interrupt_source & INTERRUPT_SOURCE_CNTPNSIRQ) {
        core_timer_disable();
        create_task(user_irq_timer_exception, 10);
        execute_tasks_preemptive();        
    }
    check_and_run_signal();
    schedule();
}

void irq_exception_handler_c(){
    // uart_send_string("IRQ Exception Occurs!\n");
    unsigned int irq = *IRQ_PENDING_1;
    unsigned int interrupt_source = *CORE0_INTERRUPT_SOURCE;

    if((irq & IRQ_PENDING_1_AUX_INT) && (interrupt_source & INTERRUPT_SOURCE_GPU) ){
        // uart_send_string("kernel UART interrupt\n");
        uart_irq_handler();
    }
    else if(interrupt_source & INTERRUPT_SOURCE_CNTPNSIRQ) {
        // uart_send_string("\nkernel Timer interrupt");
        core_timer_disable();
        create_task(irq_timer_exception, 10);
        execute_tasks_preemptive();

    }
    check_and_run_signal();
    schedule();
}

void user_exception_handler_c(trapframe_t* tf) {
    // check svc syscall from el0
    unsigned long long esr_el1 = 0;
	asm volatile("mrs %0, esr_el1":"=r"(esr_el1));
    unsigned ec = (esr_el1 >> 26) & 0x3F;

    if(ec != 0x15) {
        exception_handler_c();
        return;
    }
    int print_info = 0;
    int syscall_code = tf -> x[8];
    el1_interrupt_enable();
    switch (syscall_code) {
        case 0:
            tf -> x[0] = getpid();
            break;
        case 1:
            tf -> x[0] = uart_read(tf -> x[0], tf -> x[1]);
            break;
        case 2:
            tf -> x[0] = uart_write(tf -> x[0], tf -> x[1]);
            break;
        case 3:
            tf -> x[0] = exec((const char*)tf -> x[0], tf->x[1]);
            break;
        case 4:
            if(print_info) uart_send_string("[INFO] system call: fork\n");
            core_timer_disable();
            // el1_interrupt_disable();
            fork(tf);
            // el1_interrupt_enable();
            core_timer_enable();
            break;
        case 5:
            exit(0);
            break;
        case 6:
            tf -> x[0] = mbox_call(
                (unsigned char)tf -> x[0], (unsigned int*)tf -> x[1]
            );
            break;
        case 7:
            kill((int)tf -> x[0]);
            break;
        case 8:
            signal((int)tf -> x[0], (void*)tf -> x[1]);
            break;
        case 9:
            posix_kill((int)tf -> x[0], (int)tf -> x[1]);
            break;
        case 11:
            if(print_info) uart_send_string("[INFO] system call: open\n");
            syscall_open(tf, (const char*)tf -> x[0], tf -> x[1]);
            break;
        case 12:
            if(print_info) uart_send_string("[INFO] system call: close\n");
            syscall_close(tf, tf -> x[0]);
            break;
        case 13:
            if(print_info) uart_send_string("[INFO] system call: write\n");
            // core_timer_disable();
            // el1_interrupt_disable();
            syscall_write(tf, tf -> x[0], (const void*)tf -> x[1], tf -> x[2]);
            // el1_interrupt_enable();
            // core_timer_enable();
            break;
        case 14:
            if(print_info) uart_send_string("[INFO] system call: read\n");
            syscall_read(tf, tf -> x[0], (void*)tf -> x[1], tf -> x[2]);
            break;
        case 15:
            if(print_info) uart_send_string("[INFO] system call: mkdir\n");
            syscall_mkdir(tf, (const char*)tf -> x[0], tf -> x[1]);
            break;
        case 16:
            if(print_info) uart_send_string("[INFO] system call: mount\n");
            syscall_mount(
                tf,
                (const char*)tf -> x[0], // ignore source
                (const char*)tf -> x[1],
                (const char*)tf -> x[2],
                0, // ignore flags
                0 // ignore data
            );
            break;
        case 17:
            if(print_info) uart_send_string("[INFO] system call: chdir\n");
            syscall_chdir(tf, (const char*)tf -> x[0]);
            break;
        case 18:
            if(print_info) uart_send_string("[INFO] system call: lseek64\n");
            // core_timer_disable();
            // el1_interrupt_disable();
            syscall_lseek64(tf, tf -> x[0], tf -> x[1], tf -> x[2]);
            // el1_interrupt_enable();
            // core_timer_enable();
            break;
        case 19:
            if(print_info) uart_send_string("[INFO] system call: ioctl\n");
            syscall_ioctl(
                tf,
                tf -> x[0],
                tf -> x[1],
                tf -> x[2],
                tf -> x[3],
                tf -> x[4],
                tf -> x[5]
            );
            break;
        case 20:
            sigreturn();
            break;
    }
    el1_interrupt_disable();
    // uart_send_string("sysre\n");
    return;
}

void irq_timer_exception(){
    // uart_send_string("enter timer\n");
    core_timer_disable();
    while(timer_head){
        unsigned long long current_time;
        asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
        if(timer_head -> timeout <= current_time) {
            timer_t *timer = timer_head;
            timer -> callback(timer -> data);
            timer_head = timer_head -> next;
            kfree(timer);
        } else {
            break;
        }
    }
    // uart_send_string("exit timer\n");
    if(timer_head){
        // uart_send_string("have timer\n");
        asm volatile("msr cntp_ctl_el0, %0"::"r"(1));
        asm volatile("msr cntp_cval_el0, %0"::"r"(timer_head -> timeout));
    } else {
        // uart_send_string("no timer\n");
        asm volatile("msr cntp_ctl_el0, %0" :: "r"(0));
    }
    core_timer_enable();
}

void user_irq_timer_exception(){
     // uart_send_string("enter timer\n");
    core_timer_disable();
    while(timer_head){
        unsigned long long current_time;
        asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
        if(timer_head -> timeout <= current_time) {
            timer_t *timer = timer_head;
            timer -> callback(timer -> data);
            timer_head = timer_head -> next;
            kfree(timer);
        } else {
            break;
        }
    }
    if(timer_head){
        asm volatile("msr cntp_ctl_el0, %0"::"r"(1));
        asm volatile("msr cntp_cval_el0, %0"::"r"(timer_head -> timeout));
    } else {
        asm volatile("msr cntp_ctl_el0, %0" :: "r"(0));
    }
    core_timer_enable();
}


void core_timer_init() {
    asm volatile("msr cntp_ctl_el0, %0"::"r"(1));
    uint64_t freq;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    freq = freq << 5;
    asm volatile("msr cntp_tval_el0, %0" : : "r"(freq));
    
    *CORE0_TIMER_IRQ_CTRL = 2;

    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
}

void core_timer_enable(){
    *CORE0_TIMER_IRQ_CTRL = 2;
}

void core_timer_disable() {
    *CORE0_TIMER_IRQ_CTRL = 0;
}