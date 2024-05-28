
#include "syscall.h"
#include "peripherals/mailbox.h"
#include "arm/mmu.h"

void sys_mbox_call(TRAP_FRAME* regs) {

    U8 channel = (U8)regs->regs[0];
    void* mailbox_va = (void*) regs->regs[1];

    void* mailbox_pa = mmu_va2pa(mailbox_va);

    int result = mailbox_call_user(channel, mailbox_pa);
    if (result == 0) {  // error
        regs->regs[0] = -1;
    } else {
        regs->regs[0] = 0;
    }
}

