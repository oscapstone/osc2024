#pragma once

#include "util.hpp"

#define CORE0_IRQ_SOURCE ((addr_t)0x40000060)
#define CNTPNSIRQ_INT    (1 << 1)
#define GPU_INT          (1 << 8)

struct TrapFrame {
  uint64_t X[30];
  uint64_t lr;
  uint64_t spsr_el1;
  uint64_t elr_el1;
  uint64_t sp_el0;
  void show() const;
};

inline int get_el() {
  return read_sysreg(CurrentEL) >> 2;
}

extern "C" {
void print_exception(TrapFrame* frame, int type);
void sync_handler(TrapFrame* frame, int type);
// irq.c
void irq_handler(TrapFrame* frame, int type);
// exception.S
void set_exception_vector_table();
}
