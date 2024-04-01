#ifndef _UTLI_H
#define _UTLI_H

#define PM_PASSWORD 0x5a000000
#define PM_RSTC ((volatile unsigned int *)(MMIO_BASE + 0x0010001c))
#define PM_RSTS ((volatile unsigned int *)(MMIO_BASE + 0x00100020))
#define PM_WDOG ((volatile unsigned int *)(MMIO_BASE + 0x00100024))
#define PM_WDOG_MAGIC 0x5a000000
#define PM_RSTC_FULLRST 0x00000020

unsigned int align(unsigned int size, unsigned int s);
void align_inplace(unsigned int *size, unsigned int s);
unsigned int get_timestamp();
void print_timestamp();
void reset();
void cancel_reset();
void power_off();
void wait_cycles(int r);
void wait_usec(unsigned int n);
void print_cur_sp();
void print_cur_el();
void print_el1_sys_reg();
void exec_in_el0(void *prog_st_addr);
#endif