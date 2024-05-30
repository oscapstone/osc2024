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

enum SEGV_TYPE {
  kNone = 0,
  kIABT,
  kPC,
  kDABT,
  kSP,
  kUnknown,
};

inline const char* reason(SEGV_TYPE type) {
  switch (type) {
    case kNone:
      return "none";
    case kIABT:
      return "undefined instruction";
    case kPC:
      return "PC alignment fault";
    case kDABT:
      return "data abort";
    case kSP:
      return "SP alignment fault";
    default:
      return "unknown";
  }
}

inline int get_el() {
  return read_sysreg(CurrentEL) >> 2;
}

extern "C" {
void return_to_user(TrapFrame* frame);
void print_exception(TrapFrame* frame, int type);
void sync_handler(TrapFrame* frame, int type);
// irq.c
void irq_handler(TrapFrame* frame, int type);
// exception.S
void set_exception_vector_table();
}

void segv_handler(int el, unsigned iss, SEGV_TYPE type);
