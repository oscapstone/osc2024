/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/* Ref: // https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf */
#ifndef __GPIO_H__
#define __GPIO_H__

#define __IO            volatile

#define MMIO_BASE       0x3F000000
#define GPIO_BASE       (MMIO_BASE + 0x00200000)
#define AUX_BASE        (MMIO_BASE + 0x00215000)
#define IRQ_BASE        (MMIO_BASE + 0x0000B000)
#define EMMC_BASE       (MMIO_BASE + 0x00300000)

#define GPIO            ((gpio_t *) (GPIO_BASE))
#define AUX             ((aux_t *) (AUX_BASE))
#define IRQ             ((irq_t *) (IRQ_BASE + 0x200))
#define EMMC            ((emmc_t *) (EMMC_BASE))

typedef struct {
    __IO unsigned int GPFSEL[6];
    unsigned int reserved0;
    __IO unsigned int GPSET[2];
    unsigned int reserved1;
    __IO unsigned int GPCLR[2];
    unsigned int reserved2;
    __IO unsigned int GPLEV[2];
    unsigned int reserved3;
    __IO unsigned int GPEDS[2];
    unsigned int reserved4;
    __IO unsigned int GPREN[2];
    unsigned int reserved5;
    __IO unsigned int GPFEN[2];
    unsigned int reserved6;
    __IO unsigned int GPHEN[2];
    unsigned int reserved7;
    __IO unsigned int GPLEN[2];
    unsigned int reserved8;
    __IO unsigned int GPAREN[2];
    unsigned int reserved9;
    __IO unsigned int GPAFEN[2];
    unsigned int reserved10;
    __IO unsigned int GPPUD;
    __IO unsigned int GPPUDCLK[2];
    unsigned int reserved11;
} gpio_t;

typedef struct {
    __IO unsigned int AUX_IRQ;
    __IO unsigned int AUX_ENABLES;
    unsigned int reserved0[14];
    __IO unsigned int AUX_MU_IO_REG;
    __IO unsigned int AUX_MU_IER_REG;
    __IO unsigned int AUX_MU_IIR_REG;
    __IO unsigned int AUX_MU_LCR_REG;
    __IO unsigned int AUX_MU_MCR_REG;
    __IO unsigned int AUX_MU_LSR_REG;
    __IO unsigned int AUX_MU_MSR_REG;
    __IO unsigned int AUX_MU_SCRATCH;
    __IO unsigned int AUX_MU_CNTL_REG;
    __IO unsigned int AUX_MU_STAT_REG;
    __IO unsigned int AUX_MU_BAUD_REG;
    unsigned int reserved1[22];
    __IO unsigned int AUX_SPI0_CNTL0_REG;
    __IO unsigned int AUX_SPI0_CNTL1_REG;
    __IO unsigned int AUX_SPI0_STAT_REG;
    unsigned int reserved2;
    __IO unsigned int AUX_SPI0_IO_REG;
    __IO unsigned int AUX_SPI0_PEEK_REG;
    unsigned int reserved3[15];
    __IO unsigned int AUX_SPI1_CNTL0_REG;
    __IO unsigned int AUX_SPI1_CNTL1_REG;
    __IO unsigned int AUX_SPI1_STAT_REG;
    unsigned int reserved4;
    __IO unsigned int AUX_SPI1_IO_REG;
    __IO unsigned int AUX_SPI1_PEEK_REG;
} aux_t;

typedef struct {
    __IO unsigned int IRQ_BASIC_PENDING;
    __IO unsigned int IRQ_PENDING1;
    __IO unsigned int IRQ_PENDING2;
    __IO unsigned int FIQ_CONTROL;
    __IO unsigned int ENABLE_IRQS1;
    __IO unsigned int ENABLE_IRQS2;
    __IO unsigned int ENABLE_BASIC_IRQS;
    __IO unsigned int DISABLE_IRQS1;
    __IO unsigned int DISABLE_IRQS2;
    __IO unsigned int DISABLE_BASIC_IRQS;
} irq_t;

typedef struct {
    __IO unsigned int ARG2;
    __IO unsigned int BLOCKSIZECNT;
    __IO unsigned int ARG1;
    __IO unsigned int CMDTM;
    __IO unsigned int RESP0;
    __IO unsigned int RESP1;
    __IO unsigned int RESP2;
    __IO unsigned int RESP3;
    __IO unsigned int DATA;
    __IO unsigned int STATUS;
    __IO unsigned int CONTROL0;
    __IO unsigned int CONTROL1;
    __IO unsigned int INTERRUPT;
    __IO unsigned int IRPT_MASK;
    __IO unsigned int IRPT_EN;
    __IO unsigned int CONTROL2;
    __IO unsigned int FORCE_IRPT;
    __IO unsigned int BOOT_TIMEOUT;
    __IO unsigned int DBG_SEL;
    __IO unsigned int EXRDFIFO_CFG;
    __IO unsigned int EXRDFIFO_EN;
    __IO unsigned int TUNE_STEP;
    __IO unsigned int TUNE_STEPS_STD;
    __IO unsigned int TUNE_STEPS_DDR;
    __IO unsigned int SPI_INT_SPT;
    __IO unsigned int SLOTISR_VER;
} emmc_t;


#endif // __GPIO_H__
