#include "int/exception.hpp"

#include "int/interrupt.hpp"
#include "io.hpp"
#include "shell/shell.hpp"

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

void print_exception(ExceptionContext* context, int type) {
  kprintf_sync("(%d) %s: %s\n", type, ExceptionType[type % 4],
               ExceptionFrom[type / 4]);
  kprintf_sync("SPSR_EL1: %032lb\n", context->spsr_el1);
  kprintf_sync("ELR_EL1 : 0x%lx\n", context->elr_el1);
  kprintf_sync("ESR_EL1 : %032lb\n", context->esr_el1);

  enable_interrupt();
  shell();
}
