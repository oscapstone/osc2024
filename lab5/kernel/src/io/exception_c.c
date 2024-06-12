

#include "arm/arm.h"
#include "utils/utils.h"
#include "exception.h"
#include "trapFrame.h"
#include "utils/printf.h"
#include "arm/mmu.h"

#include "syscall/syscall.h"
#include "peripherals/irq.h"

const char entry_error_messages[16][32] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1T",		

	"SYNC_INVALID_EL1h",
	"IRQ_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		

	"SYNC_INVALID_EL0_64",		
	"IRQ_INVALID_EL0_64",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	

	"SYNC_INVALID_EL0_32",		
	"IRQ_INVALID_EL0_32",		
	"FIQ_INVALID_EL0_32",		
	"ERROR_INVALID_EL0_32"	
};

void show_invalid_entry_message(U32 type, U64 esr, U64 address, U64 spsr) {
    printf("ERROR exception: %s - %d, ESR: %X, Address: %X SPSR: %x\n", entry_error_messages[type], type, esr, address, spsr);
}

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
        printf("[ERROR] kernel mode crash(EL0 to EL1)\n");
        U64 far_el1 = utils_read_sysreg(FAR_EL1);
        U64 elr_el1 = utils_read_sysreg(ELR_EL1);
        UPTR fault_page_addr = far_el1 & ~(0xfff);
        U64 iss = ESR_ELx_ISS(esr);
        U64 sp = utils_read_genreg(sp);
        U64 sp_el0 = utils_read_sysreg(sp_el0);

        NS_DPRINT("pid:     %d\n", task_get_current_el1()->pid);
        NS_DPRINT("page:    0x%p\n", fault_page_addr);
        NS_DPRINT("far_el1: 0x%p\n", far_el1);
        NS_DPRINT("elr_el1: 0x%p\n", elr_el1);
        NS_DPRINT("esr_el1: 0x%p\n", esr);
        NS_DPRINT("sp:      0x%p\n", sp);
        NS_DPRINT("sp_el0:  0x%p\n", sp_el0);

        task_exit(-1);
    }
        break;
    }

}

void exception_sync_el1_handler(TRAP_FRAME* tf) {
    U64 esr = utils_read_sysreg(esr_el1);
    U32 ec = ESR_ELx_EC(esr);


#ifdef NS_DEBUG

    U64 far_el1 = utils_read_sysreg(FAR_EL1);
    U64 elr_el1 = utils_read_sysreg(ELR_EL1);
    UPTR fault_page_addr = far_el1 & ~(0xfff);
    U64 iss = ESR_ELx_ISS(esr);
    U64 sp = utils_read_genreg(sp);
    U64 sp_el0 = utils_read_sysreg(sp_el0);

    NS_DPRINT("[MMU][TRACE] memfail() start.\n");
    NS_DPRINT("EL1 SYNC EXCEPTION: ec: 0x%x\n", ec);
    NS_DPRINT("page:    0x%p\n", fault_page_addr);
    NS_DPRINT("far_el1: 0x%p\n", far_el1);
    NS_DPRINT("elr_el1: 0x%p\n", elr_el1);
    NS_DPRINT("esr_el1: 0x%p\n", esr);
    NS_DPRINT("sp:      0x%p\n", sp);
    NS_DPRINT("sp_el0:  0x%p\n", sp_el0);
    TASK* task = task_get_current_el1();

    NS_DPRINT("pid:     %d\n", task->pid);
#endif

    switch (ec)
    {
    case ESR_ELx_EC_IABT:
    case ESR_ELx_EC_DABT:
        mmu_memfail_handler(esr);
        break;
    default:
    {
        printf("[ERROR] kernel mode crash\n");
        task_exit(-1);
    }
        break;
    }

}
