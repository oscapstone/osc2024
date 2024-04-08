#ifndef P_MINI_UART_H
#define P_MINI_UART_H

#include "peripheral/base.h"

#define AUX_ENABLES     (PBASE + 0x00215004)
#define AUX_MU_IO_REG   (PBASE + 0x00215040)
#define AUX_MU_LCR_REG  (PBASE + 0x0021504C)
#define AUX_MU_MCR_REG  (PBASE + 0x00215050)
#define AUX_MU_LSR_REG  (PBASE + 0x00215054)
#define AUX_MU_CNTL_REG (PBASE + 0x00215060)
#define AUX_MU_BAUD_REG (PBASE + 0x00215068)


#define AUX_MU_IIR_REG (PBASE + 0x00215048)
#define INT_ID_MASK    0x6
#define NO_INT         0
#define TX_INT         (1 << 1)
#define RX_INT         (1 << 2)

#define AUX_MU_IER_REG (PBASE + 0x00215044)
#define ENABLE_RX_INT  (1 << 0)
#define ENABLE_TX_INT  (1 << 1)


#define ENABLE_IRQs_1  (PBASE + 0x0000B210)
#define ENABLE_IRQs_2  (PBASE + 0x0000B214)
#define DISABLE_IRQs_1 (PBASE + 0x0000B21C)
#define DISABLE_IRQs_2 (PBASE + 0x0000B220)

#define IRQ_PENDING_1 (PBASE + 0x0000B204)
#define IRQ_PENDING_2 (PBASE + 0x0000B208)

#define AUX_INT (1 << 29)

#endif /* P_MINI_UART_H */
