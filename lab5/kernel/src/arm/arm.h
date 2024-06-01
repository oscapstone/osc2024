#pragma once

#include "base.h"

#include "mmu.h"

// QA7_rev3.4 pg. 7
struct arm_core_regs {
    REG32 control;
    REG32 blank;
    REG32 timer_prescaler;
    REG32 gpu_int_routing;
    REG32 performance_monitor_int_routing_set;
    REG32 performance_monitor_int_routing_clear;
    REG32 blank2;
    REG32 timer_access_ls_32;
    REG32 timer_access_ms_32;
    REG32 local_int_routing;
    REG32 blank3;
    REG32 axi_outstanding_counter;
    REG32 axi_outstanding_irq;
    REG32 local_timer_control_status;
    REG32 local_timer_write_flags;
    REG32 blank4;
    REG32 timer_control[4];
    REG32 mailbox_int_control[4];
    REG32 irq_source[4];
    REG32 fiq_source[4];
    REG32 core0_mailbox_wo[4];        // write set
    REG32 core1_mailbox_wo[4];        // write set
    REG32 core2_mailbox_wo[4];        // write set
    REG32 core3_mailbox_wo[4];        // write set
};


#define REGS_ARM_CORE ((struct arm_core_regs*)(MMU_PHYS_TO_VIRT(0x40000000)))

// armv8 pg. 1899
// EC,bits[31:26]
#define ESR_ELx_EC(esr) ((esr & 0xFC000000) >> 26)
// ISS,bits[24:0]
#define ESR_ELx_ISS(esr) (esr & 0x01FFFFFF)

#define ISS_IS_WRITE(iss) (iss & 0x40)

#define ISS_TF_LEVEL0 0b000100 // iss IFSC, bits [5:0]
#define ISS_TF_LEVEL1 0b000101
#define ISS_TF_LEVEL2 0b000110
#define ISS_TF_LEVEL3 0b000111

#define ISS_PF_LEVEL0 0xc0
#define ISS_PF_LEVEL1 0xc1
#define ISS_PF_LEVEL2 0xc2
#define ISS_PF_LEVEL3 0xc3

// armv8 pg. 1900
#define ESR_ELx_EC_SVC64                0x15
#define ESR_ELx_EC_DABT_LOW             0x24
#define ESR_ELx_EC_IABT_LOW             0x20

// kernel space
#define PHYS_OFFSET 0xffff000000000000L
