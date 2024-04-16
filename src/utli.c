#include "utli.h"

#include "math_.h"
#include "mbox.h"
#include "peripherals/gpio.h"
#include "peripherals/mbox.h"
#include "peripherals/mmio.h"
#include "uart1.h"
#include "utli.h"

uint32_t align(uint32_t size, uint32_t s) {
  return (size + s - 1) & (~(s - 1));
}

void align_inplace(uint32_t *size, uint32_t s) {
  *size = ((*size) + (s - 1)) & (~(s - 1));
}

uint64_t get_clk_freq() {
  register uint64_t f;
  asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
  return f;
}

uint32_t get_timestamp() {
  register uint64_t f, c;
  asm volatile("mrs %0, cntfrq_el0"
               : "=r"(f));  // get current counter frequency
  asm volatile("mrs %0, cntpct_el0" : "=r"(c));  // read current counter
  return c / f;
}

void print_timestamp() {
  register uint64_t f, c;
  asm volatile("mrs %0, cntfrq_el0"
               : "=r"(f));  // get current counter frequency
  asm volatile("mrs %0, cntpct_el0" : "=r"(c));  // read current counter
  uart_send_string("current timestamp: ");
  uart_int(c / f);
  uart_send_string("\r\n");
}

void reset(int tick) {            // reboot after watchdog timer expire
  *PM_RSTC = PM_PASSWORD | 0x20;  // full reset
  *PM_WDOG = PM_PASSWORD | tick;  // number of watchdog tick
}

void cancel_reset() {
  *PM_RSTC = PM_PASSWORD | 0;  // full reset
  *PM_WDOG = PM_PASSWORD | 0;  // number of watchdog tick
}

void wait_cycles(uint32_t r) {
  if (r > 0) {
    while (r--) {
      asm volatile("nop");  // Execute the 'nop' instruction
    }
  }
}

/**
 * Shutdown the board
 */
void power_off() {
  uint64_t r;

  // power off devices one by one
  for (r = 0; r < 16; r++) {
    mbox[0] = 8 * 4;
    mbox[1] = MBOX_CODE_BUF_REQ;
    mbox[2] = MBOX_TAG_SET_POWER;  // set power state
    mbox[3] = 8;
    mbox[4] = MBOX_CODE_TAG_REQ;
    mbox[5] = (uint32_t)r;  // device id
    mbox[6] = 0;            // bit 0: off, bit 1: no wait
    mbox[7] = MBOX_TAG_LAST;
    mbox_call(MBOX_CH_PROP);
  }

  // power off gpio pins (but not VCC pins)
  *GPFSEL0 = 0;
  *GPFSEL1 = 0;
  *GPFSEL2 = 0;
  *GPFSEL3 = 0;
  *GPFSEL4 = 0;
  *GPFSEL5 = 0;
  *GPPUD = 0;

  wait_cycles(150);
  *GPPUDCLK0 = 0xffffffff;
  *GPPUDCLK1 = 0xffffffff;
  wait_cycles(150);
  *GPPUDCLK0 = 0;
  *GPPUDCLK1 = 0;  // flush GPIO setup

  // power off the SoC (GPU + CPU)
  r = *PM_RSTS;
  r &= ~0xfffffaaa;
  r |= 0x555;  // partition 63 used to indicate halt
  *PM_RSTS = PM_WDOG_MAGIC | r;
  *PM_WDOG = PM_WDOG_MAGIC | 10;
  *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
}

/**
 * Wait N microsec (ARM CPU only)
 */
void wait_usec(uint32_t n) {
  register uint64_t f, t, r;
  // get the current counter frequency
  asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
  // read the current counter
  asm volatile("mrs %0, cntpct_el0" : "=r"(t));
  // calculate required count increase
  uint64_t i = ((f / 1000) * n) / 1000;
  // loop while counter increase is less than i
  do {
    asm volatile("mrs %0, cntpct_el0" : "=r"(r));
  } while (r - t < i);
}

void print_cur_el() {
  uint64_t el;
  asm volatile(
      "mrs %0,CurrentEL"
      : "=r"(el));  // CurrentEL reg; bits[3:2]: current EL; bits[1:0]: reserved
  uart_send_string("current EL: ");
  uart_int((el >> 2) & 3);
  uart_send_string("\r\n");
}

void print_cur_sp() {
  uint64_t sp_val;
  asm volatile("mov %0, sp" : "=r"(sp_val));
  uart_send_string("current sp: 0x");
  uart_hex(sp_val);
  uart_send_string("\r\n");
}

void print_el1_sys_reg() {
  uint64_t spsr_el1, elr_el1, esr_el1;

  // Access the registers using inline assembly
  asm volatile("mrs %0, SPSR_EL1"
               : "=r"(spsr_el1));  // spsr_el1:holds the saved processor state
                                   // when an exception is taken to EL1
  asm volatile("mrs %0, ELR_EL1" : "=r"(elr_el1));
  asm volatile("mrs %0, ESR_EL1"
               : "=r"(esr_el1));  // esr_el1: holds syndrome information for an
                                  // exception taken to EL1
  uart_send_string(
      "SPSR_EL1 : ");  // bit[31](the N flag) may equal to 1 here, which is the
                       // nagative condition flag, indicating whether the result
                       // of an operation is negative or not.
  uart_hex(spsr_el1);
  uart_send_string("\r\n");
  uart_send_string("ELR_EL1 : ");
  uart_hex(elr_el1);
  uart_send_string("\r\n");
  uart_send_string("ESR_EL1 : ");  // EC(bits[31:26]): indicates the cause of
  uart_hex(esr_el1);               // the exception; 0x15 here -> SVC
  uart_send_string("\r\n");        // instruction from AArch64
                                   // IL(bit[25]): the instrction length bit,
                                   // for sync exceptions; is set to 1 here ->
                                   // 32-bit trapped instruction
}

void exec_in_el0(void *prog_st_addr) {
  asm volatile(
      "mov	x1, 0x0;"       // open interrupt for 2sec time_interrupt, 0000:
                                // EL0t(jump to EL0)
      "msr	spsr_el1, x1;"  // saved process state when an exception is
                                // taken to EL1
      "msr	elr_el1,  x0;"  // put program_start -> ELR_EL1
      "mov	x1, #0x20000;"  // set sp on 0x20000
      "msr	sp_el0, x1;"    // set EL0 stack pointer
      "ERET"                    // exception return
  );
  return;
}
