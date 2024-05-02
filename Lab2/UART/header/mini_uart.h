#define MMIO_BASE       0x3F000000 // Base address for memory-mapped I/O

// GPIO Function Select Registers.
#define GPFSEL1         ((volatile unsigned int*)(MMIO_BASE+0x00200004))
#define GPPUD           ((volatile unsigned int*)(MMIO_BASE+0x00200094))
#define GPPUDCLK0       ((volatile unsigned int*)(MMIO_BASE+0x00200098))

// Mini UART (AUX) Peripheral Registers.
#define AUX_ENABLE      ((volatile unsigned int *)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO_REG   ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER_REG  ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_LCR_REG  ((volatile unsigned int *)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR_REG  ((volatile unsigned int *)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR_REG  ((volatile unsigned int *)(MMIO_BASE + 0x00215054))
#define AUX_MU_CNTL_REG ((volatile unsigned int *)(MMIO_BASE + 0x00215060))
#define AUX_MU_BAUD_REG ((volatile unsigned int *)(MMIO_BASE + 0x00215068))
#define AUX_MU_IIR_REG  ((volatile unsigned int *)(MMIO_BASE + 0x00215048))

void uart_init();
void uart_send_string(const char* str);
void uart_send(char c);
char uart_recv();
char uart_recv_raw();
void uart_hex(unsigned int d);
unsigned int uart_printf(char *fmt, ...);