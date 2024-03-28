#include "uart.h"

#include "mmio.h"
// #include "stdint.h"

void uart_init() {
    uint32_t t;

    /* Init GPIO */
    // mini UART  -> set ALT5, PL011 UART -> set ALT0
    // Configure GPFSELn register to change alternate function
    t = mmio_read(GPFSEL1);
    t &= ~(7 << 12);  // clean gpio14
    t |= (2 << 12);   // set alt5
    t &= ~(7 << 15);  // clean gpio15
    t |= (2 << 15);   // set alt5
    mmio_write(GPFSEL1, t);

    /* Configure pull up/down register to disable GPIO pull up/down */
    /*
        The GPIO Pull-up/down Clock Registers control the actuation of internal pull-downs on
        the respective GPIO pins. These registers must be used in conjunction with the GPPUD
        register to effect GPIO Pull-up/down changes. The following sequence of events is
        required:
        1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
        to remove the current Pull-up/down)
        2. Wait 150 cycles – this provides the required set-up time for the control signal
        3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
        modify – NOTE only the pads which receive a clock will be modified, all others will
        retain their previous state.
        4. Wait 150 cycles – this provides the required hold time for the control signal
        5. Write to GPPUD to remove the control signal
        6. Write to GPPUDCLK0/1 to remove the clock
    */
    mmio_write(GPPUD, 0);
    delay(150);
    mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    mmio_write(GPPUDCLK0, 0);

    /* Init mini UART */
    // 0. The MMIO area base address is 0x3F000000 on raspi3
    // 1. Set AUXENB register to enable mini UART. Then mini UART register can be accessed.
    t = mmio_read(AUX_ENABLES);
    mmio_write(AUX_ENABLES, t | 1);

    // 2. Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
    mmio_write(AUX_MU_CNTL_REG, 0);

    // 3. Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
    mmio_write(AUX_MU_IER_REG, 0);

    // 4. Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
    mmio_write(AUX_MU_LCR_REG, 3);

    // 5. Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
    mmio_write(AUX_MU_MCR_REG, 0);

    // 6. Set AUX_MU_BAUD to 270. Set baud rate to 115200
    mmio_write(AUX_MU_BAUD_REG, 270);

    // 7. Set AUX_MU_IIR_REG to 6. No FIFO.
    mmio_write(AUX_MU_IIR_REG, 6);

    // 8. Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver.
    mmio_write(AUX_MU_CNTL_REG, 3);

    // clear rx data
    uart_flush();
}

void uart_write(char c) {
    while (!(mmio_read(AUX_MU_LSR_REG) & (1 << 5))) delay(1);
    mmio_write(AUX_MU_IO_REG, c);
}

void uart_read(char* buf, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        while (!(mmio_read(AUX_MU_LSR_REG) & 1)) delay(1);
        buf[i] = mmio_read(AUX_MU_IO_REG) & 0xff;
    }
}

void uart_flush() {
    while (mmio_read(AUX_MU_LSR_REG) & 1) mmio_read(AUX_MU_IO_REG);
}

void uart_write_string(char* str) {
    for (uint32_t i = 0; str[i] != '\0'; i++) {
        uart_write((char)str[i]);
    }
}

void uart_puth(uint32_t d) {
    uint32_t c;

    for (int i = 28; i >= 0; i -= 4) {
        /* Highest 4 bits */
        c = (d >> i) & 0xF;
        /* Translate to hex */
        c = (c > 9) ? (0x37 + c) : (0x30 + c);
        uart_write(c);
    }
}

void uart_putc(char* buf, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        uart_write(buf[i]);
    }
}

void delay(uint32_t t) {
    for (uint32_t i = 0; i < t; i++)
        asm volatile("nop");
}