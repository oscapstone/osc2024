#include "int/exception.hpp"

#include "arm.hpp"
#include "int/interrupt.hpp"
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

  panic("unknown exception");
}

void sync_handler(TrapFrame* frame, int type) {
  auto el = type < 8 ? 1 : 0;
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
          signal_return(frame);
          break;
      }
      break;

    case ESR_ELx_EC_IABT_LOW:
    case ESR_ELx_EC_IABT_CUR:
      segv_handler(el, iss, kIABT);
      break;

    case ESR_ELx_EC_PC_ALIGN:;
      segv_handler(el, iss, kPC);
      break;

    case ESR_ELx_EC_DABT_LOW:
    case ESR_ELx_EC_DABT_CUR:
      segv_handler(el, iss, kDABT);
      break;

    case ESR_ELx_EC_SP_ALIGN:;
      segv_handler(el, iss, kSP);
      break;

    default:
      kprintf_sync("unknown ESR_ELx_EC %06b\n type %d el %d @ 0x%lx\n", ec,
                   type, el, read_sysreg(ELR_EL1));
      segv_handler(el, iss, kUnknown);
      prog_hang();
  }
}

void segv_handler(int el, unsigned iss, SEGV_TYPE type) {
  klog("fault address: 0x%lx\n", read_sysreg(FAR_EL1));
  klog("segv_handler: iss %025b\n", iss);
  if (el != 0) {
    panic("%s", reason(type));
  } else {
    klog("thread %d: %s\n", current_thread()->tid, reason(type));
    kthread_exit(-1);
  }
}

void return_to_user(TrapFrame* frame) {
  enable_interrupt();
  current_thread()->signal.handle(frame);
  disable_interrupt();
}
