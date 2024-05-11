#include "int/exception.hpp"

#include "arm.hpp"
#include "board/pm.hpp"
#include "int/interrupt.hpp"
#include "int/timer.hpp"
#include "io.hpp"
#include "syscall.hpp"
#include "thread.hpp"

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
  kprintf("ESR_EL1 \t= 0x%lx\n", read_sysreg(ESR_EL1));
  kprintf("SP_EL0  \t= 0x%lx\n", sp_el0);
  kprintf("------------------\n");
}

void print_exception(TrapFrame* frame, int type) {
  kprintf_sync("(%d) %s: %s\n", type, ExceptionType[type % 4],
               ExceptionFrom[type / 4]);
  kprintf_sync("SPSR_EL1: %032lb\n", frame->spsr_el1);
  kprintf_sync("ELR_EL1 : 0x%lx\n", frame->elr_el1);
  kprintf_sync("ESR_EL1 : %032lb\n", read_sysreg(ESR_EL1));

  delay(freq_of_timer);
  reboot();
}

void sync_handler(TrapFrame* frame, int /*type*/) {
  unsigned long esr = read_sysreg(ESR_EL1);
  unsigned ec = ESR_ELx_EC(esr);
  unsigned iss = ESR_ELx_ISS(esr);

  switch (ec) {
    case ESR_ELx_EC_SVC64:
      // imm16
      switch (iss & MASK(16)) {
        case 0:
          syscall_handler(frame);
          break;
        case 1:
          klog("thread %d signal_return\n", current_thread()->tid);
          signal_return(const_cast<TrapFrame*>(frame));
          break;
      }
      break;

    default:
      kprintf_sync("unknown ESR_ELx_EC %06b\n", ec);
  }
}

void return_to_user(TrapFrame* frame) {
  enable_interrupt();
  current_thread()->signal.handle(frame);
  disable_interrupt();
}
