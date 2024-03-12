#pragma once
#include "util.h"

#define MMIO_BASE 0x3F000000

#define MAILBOX_BASE ((addr_t)(MMIO_BASE + 0xb880))

#define GPFSEL0   ((addr_t)(MMIO_BASE + 0x00200000))
#define GPFSEL1   ((addr_t)(MMIO_BASE + 0x00200004))
#define GPFSEL2   ((addr_t)(MMIO_BASE + 0x00200008))
#define GPFSEL3   ((addr_t)(MMIO_BASE + 0x0020000C))
#define GPFSEL4   ((addr_t)(MMIO_BASE + 0x00200010))
#define GPFSEL5   ((addr_t)(MMIO_BASE + 0x00200014))
#define GPSET0    ((addr_t)(MMIO_BASE + 0x0020001C))
#define GPSET1    ((addr_t)(MMIO_BASE + 0x00200020))
#define GPCLR0    ((addr_t)(MMIO_BASE + 0x00200028))
#define GPLEV0    ((addr_t)(MMIO_BASE + 0x00200034))
#define GPLEV1    ((addr_t)(MMIO_BASE + 0x00200038))
#define GPEDS0    ((addr_t)(MMIO_BASE + 0x00200040))
#define GPEDS1    ((addr_t)(MMIO_BASE + 0x00200044))
#define GPHEN0    ((addr_t)(MMIO_BASE + 0x00200064))
#define GPHEN1    ((addr_t)(MMIO_BASE + 0x00200068))
#define GPPUD     ((addr_t)(MMIO_BASE + 0x00200094))
#define GPPUDCLK0 ((addr_t)(MMIO_BASE + 0x00200098))
#define GPPUDCLK1 ((addr_t)(MMIO_BASE + 0x0020009C))

#define AUX_ENABLES     ((addr_t)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO_REG   ((addr_t)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER_REG  ((addr_t)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR_REG  ((addr_t)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR_REG  ((addr_t)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR_REG  ((addr_t)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR_REG  ((addr_t)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR_REG  ((addr_t)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH  ((addr_t)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL_REG ((addr_t)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT_REG ((addr_t)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD_REG ((addr_t)(MMIO_BASE + 0x00215068))
