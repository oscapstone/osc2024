#include <bsp/irq.h>
/* Auxilary mini UART registers */
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

#include <bsp/irq.h>
#include <bsp/uart.h>
#include <lib/stdlib.h>

uint32_t is_aux_irq() {
    // uart_puts("\nIRQ_PENDING_1: ");
    // uart_hex((unsigned long)*IRQ_PENDING_1);
    // uart_puts("\n");
    return (*IRQ_PENDING_1 & (1 << 29));
}

uint32_t is_uart_rx_irq() { return (*AUX_MU_IIR & 0x6) == (1 << 2); }

uint32_t is_uart_tx_irq() { return (*AUX_MU_IIR & 0x6) == (1 << 1); }
