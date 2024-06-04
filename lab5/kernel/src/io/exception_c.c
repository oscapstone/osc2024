

#include "arm/arm.h"
#include "utils/utils.h"
#include "exception.h"
#include "trapFrame.h"
#include "utils/printf.h"
#include "arm/mmu.h"

#include "syscall/syscall.h"
#include "peripherals/irq.h"

void exception_sync_el0_handler(TRAP_FRAME* trap_frame) {

    U64 esr = utils_read_sysreg(esr_el1);
    U32 ec = ESR_ELx_EC(esr);


    switch (ec)
    {
    // using svc instruction
    case ESR_ELx_EC_SVC64:
    {
        syscall_handler(trap_frame);
    }
        break;
    case ESR_ELx_EC_DABT_LOW:
    case ESR_ELx_EC_IABT_LOW:
        mmu_memfail_handler(esr);
        break;
    default:
    {
        printf("EL0 SYNC EXCEPTION: unhandle. ec: 0x%x\n", ec);
    }
        break;
    }

}

void exception_sync_el1_handler(TRAP_FRAME* tf) {
    U64 esr = utils_read_sysreg(esr_el1);
    U32 ec = ESR_ELx_EC(esr);


    switch (ec)
    {
    case ESR_ELx_EC_IABT:
    case ESR_ELx_EC_DABT:
        mmu_memfail_handler(esr);
        break;
    default:
    {
        NS_DPRINT("EL1 SYNC EXCEPTION: unhandle. ec: 0x%x\n", ec);
    }
        break;
    }

}
