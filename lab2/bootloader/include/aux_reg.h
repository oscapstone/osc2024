#ifndef	AUX_REG_H

#define	AUX_REG_H
#define MMIO_BASE       0x3F000000

#define AUX_ENABLES     (unsigned int*)(MMIO_BASE+0x00215004)
#define AUX_MU_IO_REG   (unsigned int*)(MMIO_BASE+0x00215040)
#define AUX_MU_IER_REG  (unsigned int*)(MMIO_BASE+0x00215044)
#define AUX_MU_IIR_REG  (unsigned int*)(MMIO_BASE+0x00215048)
#define AUX_MU_LCR_REG  (unsigned int*)(MMIO_BASE+0x0021504C)
#define AUX_MU_MCR_REG  (unsigned int*)(MMIO_BASE+0x00215050)
#define AUX_MU_LSR_REG  (unsigned int*)(MMIO_BASE+0x00215054)
#define AUX_MU_MSR_REG  (unsigned int*)(MMIO_BASE+0x00215058)
#define AUX_MU_SCRATCH  (unsigned int*)(MMIO_BASE+0x0021505C)
#define AUX_MU_CNTL_REG (unsigned int*)(MMIO_BASE+0x00215060)
#define AUX_MU_STAT_REG (unsigned int*)(MMIO_BASE+0x00215064)
#define AUX_MU_BAUD_REG (unsigned int*)(MMIO_BASE+0x00215068)

#endif

// Physical addresses range from 0x3F000000 to 0x3FFFFFFF for peripherals. The
// bus addresses for peripherals are set up to map onto the peripheral bus address range
// starting at 0x7E000000. Thus a peripheral advertised here at bus address 0x7Ennnnnn is
// available at physical address 0x3Fnnnnnn.
