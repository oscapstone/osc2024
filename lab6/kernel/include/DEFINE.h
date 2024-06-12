#define MMIO_BASE   0xffff00003F000000
#define PBASE MMIO_BASE

#define AUX_IRQ             ((volatile unsigned int*)(MMIO_BASE + 0x00215000))
// Auxiliary enables
#define AUX_ENABLE          ((volatile unsigned int*)(MMIO_BASE + 0x00215004))
// Mini Uart I/O Data
#define AUX_MU_IO_REG       ((volatile unsigned int*)(MMIO_BASE + 0x00215040))
// Mini Uart Interrupt Enable
#define AUX_MU_IER_REG      ((volatile unsigned int*)(MMIO_BASE + 0x00215044))
// Mini Uart Interrupt Identity
#define AUX_MU_IIR_REG      ((volatile unsigned int*)(MMIO_BASE + 0x00215048))
// Mini Uart Line Control
#define AUX_MU_LCR_REG      ((volatile unsigned int*)(MMIO_BASE + 0x0021504C))
// Mini Uart Modem Control
#define AUX_MU_MCR_REG      ((volatile unsigned int*)(MMIO_BASE + 0x00215050))
// Mini Uart Line Status
#define AUX_MU_LSR_REG      ((volatile unsigned int*)(MMIO_BASE + 0x00215054))
// Mini Uart Modem Status
#define AUX_MU_MSR_REG      ((volatile unsigned int*)(MMIO_BASE + 0x00215058))
// Mini Uart Scratch
#define AUX_MU_SCRATCH      ((volatile unsigned int*)(MMIO_BASE + 0x0021505C))
// Mini Uart Scratch
#define AUX_MU_CNTL_REG     ((volatile unsigned int*)(MMIO_BASE + 0x00215060))
// Mini Uart Extra Status
#define AUX_MU_STAT_REG     ((volatile unsigned int*)(MMIO_BASE + 0x00215064))
// Mini Uart Baudrate
#define AUX_MU_BAUD_REG     ((volatile unsigned int*)(MMIO_BASE + 0x00215068))

#define IRQ_basic_pending       ((volatile unsigned int*)(MMIO_BASE + 0x0000B200))
#define IRQ_pending_1           ((volatile unsigned int*)(MMIO_BASE + 0x0000B204))
#define IRQ_pending_2           ((volatile unsigned int*)(MMIO_BASE + 0x0000B208))
#define FIQ_control             ((volatile unsigned int*)(MMIO_BASE + 0x0000B20C))
#define Enable_IRQs_1           ((volatile unsigned int*)(MMIO_BASE + 0x0000B210))
#define Enable_IRQs_2           ((volatile unsigned int*)(MMIO_BASE + 0x0000B214))
#define Enable_Basic_IRQs       ((volatile unsigned int*)(MMIO_BASE + 0x0000B218))
#define Disable_IRQs_1          ((volatile unsigned int*)(MMIO_BASE + 0x0000B21C))
#define Disable_IRQs_2          ((volatile unsigned int*)(MMIO_BASE + 0x0000B220))
#define Disable_Basic_IRQs      ((volatile unsigned int*)(MMIO_BASE + 0x0000B224))
#define CORE0_INT_SRC           (volatile unsigned int*)(0x40000060)
#define CORE0_TIMER_IRQ_CTRL    (volatile unsigned int*)(0x40000040)

#define GPFSEL1         (PBASE+0x00200004)
#define GPSET0          (PBASE+0x0020001C)
#define GPCLR0          (PBASE+0x00200028)
#define GPPUD           (PBASE+0x00200094)
#define GPPUDCLK0       (PBASE+0x00200098)

#define UART_DR   ( (volatile unsigned int *) ( MMIO_BASE + 0x00201000 ) ) /* Data Register */
#define UART_FR   ( (volatile unsigned int *) ( MMIO_BASE + 0x00201018 ) ) /* Flag register */
#define UART_IBRD ( (volatile unsigned int *) ( MMIO_BASE + 0x00201024 ) ) /* Integer Baud Rate Divisor */
#define UART_FBRD ( (volatile unsigned int *) ( MMIO_BASE + 0x00201028 ) ) /* RFractional Baud Rate Divisor */
#define UART_LCRH ( (volatile unsigned int *) ( MMIO_BASE + 0x0020102C ) ) /* Line Control Register */
#define UART_CR   ( (volatile unsigned int *) ( MMIO_BASE + 0x00201030 ) ) /* Control Register */
#define UART_RIS  ( (volatile unsigned int *) ( MMIO_BASE + 0x0020103C ) )
#define UART_IMSC ( (volatile unsigned int *) ( MMIO_BASE + 0x00201038 ) ) /* Interupt FIFO Level Select Register */
#define UART_MIS  ( (volatile unsigned int *) ( MMIO_BASE + 0x00201040 ) )
#define UART_ICR  ( (volatile unsigned int *) ( MMIO_BASE + 0x00201044 ) ) /* Interupt Clear Register */
