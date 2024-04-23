#include "exception.h"
#include "uart.h"

void init_exception_vectors(void)
{
    // set EL1 exception vector table
    asm("adr x0, exception_vector_table");
    asm("msr vbar_el1, x0");
}

void init_interrupt(void)
{
    _init_core_timer();
}

void _init_core_timer(void)
{
    // enable timer
    asm("mov x0, 1");
    asm("msr cntp_ctl_el0, x0");    // enable timer interrupt (timer control)

    // set expired time
    asm("mrs x0, cntfrq_el0");  // interrupt in 1 sec (as the lab website)
    asm("msr cntp_tval_el0, x0");

    // unmask timer interrupt
    asm("mov x0, 2");
    asm("ldr x1, =%0" : : "i"(CORE0_TIMER_IRQ_CTRL));
    asm("str x0, [x1]");
}

void handle_exception(void)
{
    uart_puts("exception raised!\n");

    long spsr_el1, elr_el1, esr_el1;
    asm("mrs %0, spsr_el1" : "=r"((long) spsr_el1));
    asm("mrs %0, elr_el1" : "=r"((long) elr_el1));
    asm("mrs %0, esr_el1" : "=r"((long) esr_el1));

    uart_puts("spsr_el1: ");
    uart_hex(spsr_el1);
    uart_puts("\n");

    uart_puts("elr_el1: ");
    uart_hex(elr_el1);
    uart_puts("\n");

    uart_puts("esr_el1: ");
    uart_hex(esr_el1);
    uart_puts("\n");

    uart_puts("\n");
}

void handle_interrupt(void)
{
    // seconds after booting
    long count;
    long freq; //62500000 (62.5 MHz)

    asm("mrs %0, cntpct_el0" : "=r"((long) count));
    asm("mrs %0, cntfrq_el0" : "=r"((long) freq));

    uart_putints(count/freq);
    uart_puts(" seconds after booting...\n");

    // set next timeout
    asm("mov x0, %0" : : "r"(freq * 1));
    asm("msr cntp_tval_el0, x0");
}

void el2_to_el1(void)
{
    // Ref: https://developer.arm.com/documentation/den0024/a/ARMv8-Registers/Processor-state
    asm("mov x0, (1 << 31)");   // EL1 uses aarch64
    asm("msr hcr_el2, x0");
    asm("mov x0, 0x3c5");       // EL1h with interrupt disabled
    asm("msr spsr_el2, x0");
    asm("msr elr_el2, lr");     // link register (subroutine return addr)
    asm("eret");
}

void el1_to_el0(void)
{
    // REF: https://gcc.gnu.org/onlinedocs/gcc/extensions-to-the-c-language-family/how-to-use-inline-assembly-language-in-c-code.html#extended-asm-assembler-instructions-with-c-expression-operands
    // Output/Input Operands
    uart_puts("EL1 to EL0\n");

    // enable irq: 0x000, disable irq: 0x3c0
    asm("mov x0, 0x000");
    asm("msr spsr_el1, x0");                                // state
    asm("msr elr_el1, %0" : : "r" (&__userspace_start));    // return address
    asm("msr sp_el0, %0" : : "r" (&__userspace_end));       // stack pointer
    asm("eret");
}