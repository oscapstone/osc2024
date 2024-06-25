#pragma once

#include "start.h"

#define VM 0xFFFF000000000000

#define TO_VIRT(x) ((unsigned long long)(x) | VM)
#define TO_PHYS(x) ((unsigned long long)(x) & ~VM)

#define MMIO_BASE TO_VIRT(0x3F000000)

// ARM Interrupt Registers

#define IRQ_BASIC_PENDING (volatile unsigned int *)(MMIO_BASE + 0x0000B200)
#define IRQ_PENDING_1 (volatile unsigned int *)(MMIO_BASE + 0x0000B204)
#define IRQ_PENDING_2 (volatile unsigned int *)(MMIO_BASE + 0x0000B208)
#define FIQ_CONTROL (volatile unsigned int *)(MMIO_BASE + 0x0000B20C)
#define ENABLE_IRQS_1 (volatile unsigned int *)(MMIO_BASE + 0x0000B210)
#define ENABLE_IRQS_2 (volatile unsigned int *)(MMIO_BASE + 0x0000B214)
#define ENABLE_BASIC_IRQS (volatile unsigned int *)(MMIO_BASE + 0x0000B218)
#define DISABLE_IRQS_1 (volatile unsigned int *)(MMIO_BASE + 0x0000B21C)
#define DISABLE_IRQS_2 (volatile unsigned int *)(MMIO_BASE + 0x0000B220)
#define DISABLE_BASIC_IRQS (volatile unsigned int *)(MMIO_BASE + 0x0000B224)

// Timers interrupt control registers

#define CORE0_INTERRUPT_SOURCE (volatile unsigned int *)TO_VIRT(0x40000060)
#define CORE0_TIMER_IRQCNTL TO_VIRT(0x40000040)
#define CORE1_TIMER_IRQCNTL TO_VIRT(0x40000044)
#define CORE2_TIMER_IRQCNTL TO_VIRT(0x40000048)
#define CORE3_TIMER_IRQCNTL TO_VIRT(0x4000004C)

// Where to route timer interrupt to, IRQ/FIQ
// Setting both the IRQ and FIQ bit gives an FIQ
#define TIMER0_IRQ 0x01
#define TIMER1_IRQ 0x02
#define TIMER2_IRQ 0x04
#define TIMER3_IRQ 0x08
#define TIMER0_FIQ 0x10
#define TIMER1_FIQ 0x20
#define TIMER2_FIQ 0x40
#define TIMER3_FIQ 0x80

// PASSWORD

#define PM_PASSWORD 0x5A000000
#define PM_RSTC (volatile unsigned int *)0x3F10001C
#define PM_WDOG (volatile unsigned int *)0x3F100024

// DEVICETREE

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

// GPIO

#define GPFSEL0 ((volatile unsigned int *)(MMIO_BASE + 0x00200000))
#define GPFSEL1 ((volatile unsigned int *)(MMIO_BASE + 0x00200004))
#define GPFSEL2 ((volatile unsigned int *)(MMIO_BASE + 0x00200008))
#define GPFSEL3 ((volatile unsigned int *)(MMIO_BASE + 0x0020000C))
#define GPFSEL4 ((volatile unsigned int *)(MMIO_BASE + 0x00200010))
#define GPFSEL5 ((volatile unsigned int *)(MMIO_BASE + 0x00200014))
#define GPSET0 ((volatile unsigned int *)(MMIO_BASE + 0x0020001C))
#define GPSET1 ((volatile unsigned int *)(MMIO_BASE + 0x00200020))
#define GPCLR0 ((volatile unsigned int *)(MMIO_BASE + 0x00200028))
#define GPCLR1 ((volatile unsigned int *)(MMIO_BASE + 0x0020002C))
#define GPLEV0 ((volatile unsigned int *)(MMIO_BASE + 0x00200034))
#define GPLEV1 ((volatile unsigned int *)(MMIO_BASE + 0x00200038))
#define GPEDS0 ((volatile unsigned int *)(MMIO_BASE + 0x00200040))
#define GPEDS1 ((volatile unsigned int *)(MMIO_BASE + 0x00200044))
// Skip GPRENn, GPFENn
#define GPHEN0 ((volatile unsigned int *)(MMIO_BASE + 0x00200064))
#define GPHEN1 ((volatile unsigned int *)(MMIO_BASE + 0x00200068))
// Skip GPLENn, GPARENn
#define GPPUD ((volatile unsigned int *)(MMIO_BASE + 0x00200094))
#define GPPUDCLK0 ((volatile unsigned int *)(MMIO_BASE + 0x00200098))
#define GPPUDCLK1 ((volatile unsigned int *)(MMIO_BASE + 0x0020009C))

// UART

#define AUX_ENABLE ((volatile unsigned int *)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR ((volatile unsigned int *)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR ((volatile unsigned int *)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR ((volatile unsigned int *)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR ((volatile unsigned int *)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH ((volatile unsigned int *)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL ((volatile unsigned int *)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT ((volatile unsigned int *)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD ((volatile unsigned int *)(MMIO_BASE + 0x00215068))

// MAILBOX

#define MAILBOX_BASE (MMIO_BASE + 0x0000B880)

// mailbox register
#define MAILBOX_REG_READ ((volatile unsigned int *)(MAILBOX_BASE + 0x00000000))
#define MAILBOX_REG_STATUS \
  ((volatile unsigned int *)(MAILBOX_BASE + 0x00000018))
#define MAILBOX_REG_WRITE ((volatile unsigned int *)(MAILBOX_BASE + 0x00000020))

// mailbox channel
// https://github.com/raspberrypi/firmware/wiki/Mailboxes#channels
#define MAILBOX_CH_POWER 0
#define MAILBOX_CH_FB 1
#define MAILBOX_CH_VUART 2
#define MAILBOX_CH_VCHIQ 3
#define MAILBOX_CH_LEDS 4
#define MAILBOX_CH_BTNS 5
#define MAILBOX_CH_TOUCH 6
#define MAILBOX_CH_COUNT 7
#define MAILBOX_CH_PROP 8

// mailbox status
#define MAILBOX_EMPTY 0x40000000
#define MAILBOX_FULL 0x80000000
#define MAILBOX_REQUEST 0x00000000
#define MAILBOX_RESPONSE 0x80000000
#define REQUEST_CODE 0x00000000
#define REQUEST_SUCCEED 0x80000000
#define REQUEST_FAILED 0x80000001
#define TAG_REQUEST_CODE 0x00000000
#define END_TAG 0x00000000

// tags
#define TAGS_REQ_CODE 0x00000000
#define TAGS_REQ_SUCCEED 0x80000000
#define TAGS_REQ_FAILED 0x80000001
#define TAGS_END 0x00000000

// hardware tags operator
#define TAGS_HARDWARE_BOARD_MODEL 0x00010001
#define TAGS_HARDWARE_BOARD_REVISION 0x00010002
#define TAGS_HARDWARE_MAC_ADDR 0x00010003
#define TAGS_HARDWARE_BOARD_SERIAL 0x00010004
#define TAGS_HARDWARE_ARM_MEM 0x00010005
#define TAGS_HARDWARE_VC_MEM 0x00010006
#define TAGS_HARDWARE_CLOCKS 0x00010007

// CLOCK ID

#define CLOCK_ID_RESERVED 0x000000000
#define CLOCK_ID_EMMC 0x000000001
#define CLOCK_ID_UART 0x000000002
#define CLOCK_ID_ARM 0x000000003
#define CLOCK_ID_CORE 0x000000004
#define CLOCK_ID_V3D 0x000000005
#define CLOCK_ID_H264 0x000000006
#define CLOCK_ID_ISP 0x000000007
#define CLOCK_ID_SDRAM 0x000000008
#define CLOCK_ID_PIXEL 0x000000009
#define CLOCK_ID_PWM 0x00000000a
#define CLOCK_ID_HEVC 0x00000000b
#define CLOCK_ID_EMMC2 0x00000000c
#define CLOCK_ID_M2MC 0x00000000d
#define CLOCK_ID_PIXEL_BVB 0x00000000e

// clock tags operator
#define TAGS_GET_CLOCK 0x00030002
#define TAGS_SET_CLOCK 0x00038002

// framebuffer tages operator
#define FB_ALLOC_BUFFER 0x00040001
#define FB_FREE_BUFFER 0x00048001

#define FB_BLANK_SCREEN 0x00040002

#define FB_PHY_WID_HEIGHT_GET 0x00040003
#define FB_PHY_WID_HEIGHT_TEST 0x00044003
#define FB_PHY_WID_HEIGHT_SET 0x00048003

#define FB_VIR_WID_HEIGHT_GET 0x00040004
#define FB_VIR_WID_HEIGHT_TEST 0x00044004
#define FB_VIR_WID_HEIGHT_SET 0x00048004

#define FB_DEPTH_GET 0x00040005
#define FB_DEPTH_TEST 0x00044005
#define FB_DEPTH_SET 0x00048005

#define FB_DEPTH_GET 0x00040005
#define FB_DEPTH_TEST 0x00044005
#define FB_DEPTH_SET 0x00048005

#define FB_PIXEL_ORDER_GET 0x00040006
#define FB_PIXEL_ORDER_TEST 0x00044006
#define FB_PIXEL_ORDER_SET 0x00048006

#define FB_ALPHA_MODE_GET 0x00040007
#define FB_ALPHA_MODE_TEST 0x00044007
#define FB_ALPHA_MODE_SET 0x00048007

#define FB_PITCH_GET 0x00040008

#define FB_VIR_OFFSET_GET 0x00040009
#define FB_VIR_OFFSET_TEST 0x00044009
#define FB_VIR_OFFSET_SET 0x00048009

#define FB_OVERSCAN_GET 0x0004000A
#define FB_OVERSCAN_TEST 0x0004400A
#define FB_OVERSCAN_SET 0x0004800A

#define FB_PALETTE_GET 0x0004000B
#define FB_PALETTE_TEST 0x0004400B
#define FB_PALETTE_SET 0x0004800B

#define FB_CURSOR_INFO_SET 0x00008010
#define FB_CURSOR_STATE_SET 0x00008011
