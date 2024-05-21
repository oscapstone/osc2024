
#include "irq.h"
#include "uart.h"
#include "sched.h"
#include "timer.h"
#include "task.h"
#include "signal.h"
#include "syscall.h"

#define AUX_MU_IIR  ((volatile unsigned int*)(MMIO_BASE+0x00215048))



#define UART_IRQ_PRIORITY 4
#define TIMER_IRQ_PRIORITY 3

// unmask specific interrupt
void enable_interrupt() {
    //uart_puts("ei daif\n");
    asm volatile("msr daifclr, 0xf");
    //uart_puts("eiii daif\n");
}
// mask specific interrupt
void disable_interrupt() {
    asm volatile("msr daifset, 0xf");
}

void irq_handler()
{
    // core0_int_src : 0x40000060
    // from aux && from GPU0 -> uart exception
    // determine GPU interrupt : bit 8
    // arm peripherals interrupt table
    // bit 29 AUX interrupt
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) 
    {   
        // bit[2:1] 01 tx
        if (*AUX_MU_IIR & (0b01 << 1)) // TX can get data from tx buffer
        {
            disable_mini_uart_tx_interrupt();
            add_task(uart_tx_interrupt_handler, UART_IRQ_PRIORITY);
            pop_task();
        }
        // bit[2:1] 10 rx
        else if (*AUX_MU_IIR & (0b10 << 1))  // kernel can get data from rx buffer
        {
            disable_mini_uart_rx_interrupt();
            add_task(uart_rx_interrupt_handler, UART_IRQ_PRIORITY);
            pop_task();
        }
        else
            uart_printf("uart handler error\n");
    }
    // check bit 1 to determine timer interrupt
    else if (*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ)
    {
        core_timer_interrupt_disable();
        // uart_printf("a1\n");
        add_task(core_timer_handler, TIMER_IRQ_PRIORITY);
        // uart_printf("a2\n");
        pop_task();
        // uart_printf("a3\n");
        core_timer_interrupt_enable();

        // timer interrupt to be round robin
        if (run_queue->next->next != run_queue) // runqueue size > 1
            schedule();
    }
}
void invalid_exception_handler(unsigned long long x0) {
    uart_printf("invalid exception : 0x%x\n", x0);
    uart_getc();
}

void cpacr_el1_off(){
    // bit 21 20 set 11
    asm volatile(
        "mov x20, (3 << 20)\n\t"
        "msr CPACR_EL1, x20\n\t"
    );
}

void sync_el0_64_handler(trapframe_t *tpf) {
    enable_interrupt();
    // get trapframe x8
    // which is system call number
    unsigned long long syscall_no = tpf->x8;

    // by lab given spec 
    // arguments store in x0 x1 x2 ...
    switch (syscall_no)
    {
    case 0:
        getpid(tpf);
        break;
    case 1:
        // buffer, size
        uartread(tpf, (char *)tpf->x0, tpf->x1);
        break;
    case 2:
        // buffer, size
        uartwrite(tpf, (char *)tpf->x0, tpf->x1);
        break;
    case 3:
        // name, argv
        exec(tpf, (char *)tpf->x0, (char **)tpf->x1);
        break;
    case 4:
        fork(tpf);
        break;
    case 5:
        // status
        // status not in use currently
        exit(tpf, tpf->x0);
        break;
    case 6:
        // channel, mbox
        syscall_mbox_call(tpf, (unsigned char)tpf->x0, (unsigned int *)tpf->x1);
        break;
    case 7:
        // pid
        kill(tpf, (int)tpf->x0);
        break;
    case 8:
        // callback
        signal_register(tpf->x0, (signal_handler_t)tpf->x1);
        break;
    case 9:
        // pid
        signal_kill(tpf->x0, tpf->x1);
        break;
    case 50:
        // self-defined signal return
        sigreturn(tpf);
        break;
    }
}


void highp() {
    uart_async_printf("high prior start\n");
    uart_async_printf("high prior end\n");

}

void lowp() {
    uart_async_printf("low prior start\n");
    add_task(highp, 0);
    uart_async_putc('\r');  // to trigger pop_task
    for(int i = 0; i < 100000; ++i);
    uart_async_printf("low prior end\n");
     for(int i = 0; i < 100000; ++i);
}

void test_preemption() {
    uart_async_printf("Starting test :\n");
    add_task(lowp, 9);
    uart_async_putc('\r');    // to trigger pop_task
}

static unsigned long long lock_count = 0;
void lock()
{
    disable_interrupt();
    lock_count++;
}

void unlock()
{
    lock_count--;
    if (lock_count < 0)
    {
        uart_printf("lock error !!!\r\n");
        while(1);
    }
    if (lock_count == 0)
        enable_interrupt();
}