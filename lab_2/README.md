1. python UART transfer data loss
- AUX_MU_LSR_REG bit 1: This bit is set if there was a receiver overrun. That is: 
one or more characters arrived whilst the receive 
FIFO was full. The newly arrived charters have been 
discarded. This bit is cleared each time this register is
read. To do a non-destructive read of this overrun bit 
use the Mini Uart Extra Status register. 
[reference: P15](https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf)

2. init UART register to avoid garbage pending
3. link loader can arrange the .text section freely
    - why align
4. wze is for halting CPU that is not id 0
5. 