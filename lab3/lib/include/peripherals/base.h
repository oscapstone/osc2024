#ifndef	_P_BASE_H
#define	_P_BASE_H

#define PBASE (volatile unsigned long)0x3F000000
/* For asynchronous uart */
#define AUX_ENABLES     ((volatile unsigned long)PBASE+0x00215004)
#define AUX_MU_IO_REG   ((volatile unsigned long)PBASE+0x00215040)
#define AUX_MU_IER_REG  ((volatile unsigned long)PBASE+0x00215044) // Mini Uart Interrupt Enable
#define AUX_MU_IIR_REG  ((volatile unsigned long)PBASE+0x00215048) // Mini Uart Interrupt Identify
#define AUX_MU_LCR_REG  ((volatile unsigned long)PBASE+0x0021504C) // Mini Uart Line Status 
#define AUX_MU_MCR_REG  ((volatile unsigned long)PBASE+0x00215050)
#define AUX_MU_LSR_REG  ((volatile unsigned long)PBASE+0x00215054)
#define AUX_MU_MSR_REG  ((volatile unsigned long)PBASE+0x00215058)
#define AUX_MU_SCRATCH  ((volatile unsigned long)PBASE+0x0021505C)
#define AUX_MU_CNTL_REG ((volatile unsigned long)PBASE+0x00215060)
#define AUX_MU_STAT_REG ((volatile unsigned long)PBASE+0x00215064)
#define AUX_MU_BAUD_REG ((volatile unsigned long)PBASE+0x00215068)

/* For exception */
#define CORE0_IRQ_SOURCE    (volatile unsigned long)0x40000060
#define IRQ_BASIC_PENDING	((volatile unsigned long)PBASE+0x0000B200)
#define IRQ_PENDING_1		((volatile unsigned long)PBASE+0x0000B204)
#define IRQ_PENDING_2		((volatile unsigned long)PBASE+0x0000B208)
#define FIQ_CONTROL		    ((volatile unsigned long)PBASE+0x0000B20C)
#define ENABLE_IRQS_1		((volatile unsigned long)PBASE+0x0000B210) // 1<<29, enable timer interrupts
#define ENABLE_IRQS_2		((volatile unsigned long)PBASE+0x0000B214)
#define ENABLE_BASIC_IRQS	((volatile unsigned long)PBASE+0x0000B218)
#define DISABLE_IRQS_1		((volatile unsigned long)PBASE+0x0000B21C)
#define DISABLE_IRQS_2		((volatile unsigned long)PBASE+0x0000B220)
#define DISABLE_BASIC_IRQS	((volatile unsigned long)PBASE+0x0000B224)

#define SYSTEM_TIMER_IRQ_0	(1 << 0)
#define SYSTEM_TIMER_IRQ_1	(1 << 1)
#define SYSTEM_TIMER_IRQ_2	(1 << 2)
#define SYSTEM_TIMER_IRQ_3	(1 << 3)

#endif  /*_P_BASE_H */
