#include "peripheral/mini_uart.h"
#include "peripheral/gpio.h"
#include "utils.h"

void uart_init(void)
{
    unsigned int gpfsel1 = get32(GPFSEL1);
    unsigned int mask = (1 << 12) | (1 << 15);
    gpfsel1 &= ~((mask << 3) - mask);  // clear gpio 14, 15
    gpfsel1 |= (mask << 1);  // set alt5 function for gpio 14, 15
    put32(GPFSEL1, gpfsel1);

    // enable gpio 14, 15
    put32(GPPUD, 0);
    delay(150);
    put32(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    put32(GPPUDCLK0, 0);  // flush GPIO setup

    put32(AUX_ENABLES, 1);      // enable mini uart
    put32(AUX_MU_CNTL_REG, 0);  // disable auto flow control and disable Tx, Rx
    put32(AUX_MU_IER_REG, 0);   // disable receive and transmit interrupts
    put32(AUX_MU_LCR_REG, 3);   // enable 8 bit mode
    put32(AUX_MU_MCR_REG, 0);   // set RTS line to be always high
    put32(AUX_MU_BAUD_REG, 270);  // 115200 baud
    put32(AUX_MU_IIR_REG, 0xC6);  // no FIFO
    put32(AUX_MU_CNTL_REG, 3);    // enable Tx, Rx
}

void uart_send(char c)
{
    while (!(get32(AUX_MU_LSR_REG) & 0x20))
        ;
    put32(AUX_MU_IO_REG, c);
}

char uart_recv(void)
{
    while (!(get32(AUX_MU_LSR_REG) & 0x01))
        ;
    char r = get32(AUX_MU_IO_REG) & 0xFF;
    return (r == '\r') ? '\n' : r;
}

void uart_send_string(const char* str)
{
    while (*str) {
        if (*str == '\n')
            uart_send('\r');
        uart_send(*str++);
    }
}

/**
 * Send a 32-bit integer in its hexadecimal form.
 */
void uart_send_hex(unsigned int data)
{
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        n = (data >> c) & 0xF;
        n += (n < 10) ? '0' : ('A' - 10);
        uart_send(n);
    }
}

/**
 * Send a 32-bit integer in its decimal form.
 */
void uart_send_dec(unsigned int data)
{
    if (data == 0) {
        uart_send('0');
        return;
    }

    int buffer[10];
    int i = 0;
    while (data > 0) {
        buffer[i++] = data % 10;
        data /= 10;
    }

    for (i = i - 1; i >= 0; i--)
        uart_send(buffer[i] + '0');
}

void uart_send_space_level(unsigned int level)
{
    for (unsigned int i = 0; i < level; i++)
        uart_send(' ');
}

void uart_enable_interrupt(void)
{
    put32(ENABLE_IRQs_1, IRQ_PENDING_1_AUX_INT);
}


void uart_disable_interrupt(void)
{
    put32(DISABLE_IRQs_1, IRQ_PENDING_1_AUX_INT);
}

void set_tx_interrupt(void)
{
    unsigned int aux_mu_ier_reg = get32(AUX_MU_IER_REG);
    aux_mu_ier_reg |= ENABLE_TX_INT;
    put32(AUX_MU_IER_REG, aux_mu_ier_reg);
}

void clear_tx_interrupt(void)
{
    unsigned int aux_mu_ier_reg = get32(AUX_MU_IER_REG);
    aux_mu_ier_reg &= ~ENABLE_TX_INT;
    put32(AUX_MU_IER_REG, aux_mu_ier_reg);
}

void set_rx_interrupt(void)
{
    unsigned int aux_mu_ier_reg = get32(AUX_MU_IER_REG);
    aux_mu_ier_reg |= ENABLE_RX_INT;
    put32(AUX_MU_IER_REG, aux_mu_ier_reg);
}

void clear_rx_interrupt(void)
{
    unsigned int aux_mu_ier_reg = get32(AUX_MU_IER_REG);
    aux_mu_ier_reg &= ~ENABLE_RX_INT;
    put32(AUX_MU_IER_REG, aux_mu_ier_reg);
}

#define UART_BUFFER_SIZE 0x100  // must be power of 2

static char uart_read_buffer[UART_BUFFER_SIZE];
static char uart_write_buffer[UART_BUFFER_SIZE];

static int uart_read_head, uart_read_idx = 0;
static int uart_write_head, uart_write_idx = 0;

#define write_buffer_empty() (uart_write_head == uart_write_idx)
#define read_buffer_empty()  (uart_read_head == uart_read_idx)

/* GNU extension - statement expression */
#define write_buffer_get()                             \
    ({                                                 \
        char c = uart_write_buffer[uart_write_head++]; \
        uart_write_head &= (UART_BUFFER_SIZE - 1);     \
        c;                                             \
    })
#define read_buffer_get()                            \
    ({                                               \
        char c = uart_read_buffer[uart_read_head++]; \
        uart_read_head &= (UART_BUFFER_SIZE - 1);    \
        c;                                           \
    })

#define write_buffer_add(c)                       \
    do {                                          \
        uart_write_buffer[uart_write_idx++] = c;  \
        uart_write_idx &= (UART_BUFFER_SIZE - 1); \
    } while (0);

#define read_buffer_add(c)                       \
    do {                                         \
        uart_read_buffer[uart_read_idx++] = c;   \
        uart_read_idx &= (UART_BUFFER_SIZE - 1); \
    } while (0);

char uart_recv_async(void)
{
    set_rx_interrupt();
    while (read_buffer_empty())
        ;
    uart_disable_interrupt();
    char c = read_buffer_get();
    uart_enable_interrupt();
    return c == '\r' ? '\n' : c;
}

void uart_send_async(char c)
{
    write_buffer_add(c);
    set_tx_interrupt();
}

void uart_send_string_async(const char* str)
{
    while (*str) {
        if (*str == '\n')
            uart_send_async('\r');
        uart_send_async(*str++);
    }
}

void uart_rx_handle_irq(void)
{
    read_buffer_add(uart_recv());
    set_rx_interrupt();
}

void uart_tx_handle_irq(void)
{
    if (write_buffer_empty()) {
        clear_tx_interrupt();
        return;
    }
    while (!write_buffer_empty()) {
        delay(1 << 15);
        char c = write_buffer_get();
        uart_send(c);
    }
}

void test_uart_async(void)
{
    uart_enable_interrupt();
    char c = uart_recv_async();
    while (c != '\n') {
        write_buffer_add(c);
        c = uart_recv_async();
    }
    write_buffer_add('\r');
    write_buffer_add(c);
    set_tx_interrupt();
    uart_disable_interrupt();
}
