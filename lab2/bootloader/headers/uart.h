#ifndef __UART_H_
#define __UART_H_

#define PHY_BASE        0x3f000000
#define BUS_BASE        0x7e000000

#define AUX_IRQ         (volatile unsigned int*)(PHY_BASE+0x00215000)
#define AUX_ENABLES     (volatile unsigned int*)(PHY_BASE+0x00215004)
#define AUX_MU_IO_REG   (volatile unsigned int*)(PHY_BASE+0x00215040)
#define AUX_MU_IER_REG  (volatile unsigned int*)(PHY_BASE+0x00215044)
#define AUX_MU_IIR_REG  (volatile unsigned int*)(PHY_BASE+0x00215048)
#define AUX_MU_LCR_REG  (volatile unsigned int*)(PHY_BASE+0x0021504c)
#define AUX_MU_MCR_REG  (volatile unsigned int*)(PHY_BASE+0x00215050)
#define AUX_MU_LSR_REG  (volatile unsigned int*)(PHY_BASE+0x00215054)
#define AUX_MU_MSR_REG  (volatile unsigned int*)(PHY_BASE+0x00215058)
#define AUX_MU_SCRATCH  (volatile unsigned int*)(PHY_BASE+0x0021505c)
#define AUX_MU_CNTL_REG (volatile unsigned int*)(PHY_BASE+0x00215060)
#define AUX_MU_STAT_REG (volatile unsigned int*)(PHY_BASE+0x00215064)
#define AUX_MU_BAUD_REG (volatile unsigned int*)(PHY_BASE+0x00215068)

#define GPFSEL1         (volatile unsigned int*)(PHY_BASE+0x00200004)
#define GPPUD           (volatile unsigned int*)(PHY_BASE+0x00200094)
#define GPPUDCLK0       (volatile unsigned int*)(PHY_BASE+0x00200098)
#define GPPUDCLK1       (volatile unsigned int*)(PHY_BASE+0x0020009c)

void init(void);
char receive(void);
void send(char);
void display(char*);

#endif
