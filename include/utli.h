#ifndef _UTLI_H
#define _UTLI_H

#include "types.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC ((volatile uint32_t *)(MMIO_BASE + 0x0010001c))
#define PM_RSTS ((volatile uint32_t *)(MMIO_BASE + 0x00100020))
#define PM_WDOG ((volatile uint32_t *)(MMIO_BASE + 0x00100024))
#define PM_WDOG_MAGIC 0x5a000000
#define PM_RSTC_FULLRST 0x00000020
#define UNUSED(x) (void)(x)

uint32_t align(uint32_t size, uint32_t s);
void align_inplace(uint32_t *size, uint32_t s);
uint32_t get_timestamp();
uint64_t get_clk_freq();
void print_timestamp();
void reset();
void cancel_reset();
void power_off();
void wait_cycles(uint32_t r);
void wait_usec(uint32_t n);
void *get_cur_sp();
void print_cur_sp();
void print_cur_el();
void print_el1_sys_reg();
void exec_in_el0(void *prog_st_addr, void *stk_ptr);
void enable_EL0VCTEN();
void print_DAIF();
void print_ttbr0_el1();
#endif