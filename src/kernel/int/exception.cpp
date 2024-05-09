#include "int/exception.hpp"

#include "arm.hpp"
#include "int/interrupt.hpp"
#include "io.hpp"
#include "shell/shell.hpp"
#include "syscall.hpp"

const char* ExceptionFrom[] = {
    "Current Exception level with SP_EL0.",
    "Current Exception level with SP_ELx, x>0.",
    "Lower Exception level, using AArch64",
    "Lower Exception level, using AArch32",
};
const char* ExceptionType[] = {
    "Synchronous",
    "IRQ or vIRQ",
    "FIQ or vFIQ",
    "SError or vSError",
};

void TrapFrame::show() const {
  kprintf("=== Trap Frame ===\n");
  for (int i = 0; i <= 30; i++)
    kprintf("x%d      \t= 0x%lx\n", i, X[i]);
  kprintf("LR      \t= 0x%lx\n", lr);
  kprintf("SPSR_EL1\t= 0x%lx\n", spsr_el1);
  kprintf("ELR_EL1 \t= 0x%lx\n", elr_el1);
  kprintf("ESR_EL1 \t= 0x%lx\n", esr_el1);
  kprintf("SP_EL0  \t= 0x%lx\n", sp_el0);
  kprintf("------------------\n");
}

void print_exception(TrapFrame* frame, int type) {
  kprintf_sync("(%d) %s: %s\n", type, ExceptionType[type % 4],
               ExceptionFrom[type / 4]);
  kprintf_sync("SPSR_EL1: %032lb\n", frame->spsr_el1);
  kprintf_sync("ELR_EL1 : 0x%lx\n", frame->elr_el1);
  kprintf_sync("ESR_EL1 : %032lb\n", frame->esr_el1);

  enable_interrupt();
  shell(nullptr);
}

void sync_handler(TrapFrame* frame, int /*type*/) {
  int ec = ESR_ELx_EC(frame->esr_el1);
  switch (ec) {
    case ESR_ELx_EC_SVC64:
      syscall_handler(frame);
      break;
    default:
      kprintf_sync("unknown ESR_ELx_EC %x\n", ec);
  }
}
