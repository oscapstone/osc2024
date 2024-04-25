// BCM2837 ARM Peripherals Page 6
#define MMIO_BASE \
    0x3F000000 
// BCM2835 page. 112-116
#define IRQ_BASIC_PENDING ((volatile unsigned int*)(MMIO_BASE + 0x0000B200))
#define IRQ_PENDING_1                     \
    ((volatile unsigned int*)(MMIO_BASE + \
                              0x0000B204))  // Holds All interrupts 0...31 from
                                            // the GPU side.
#define IRQ_PENDING_2 \
    ((volatile unsigned int*)(MMIO_BASE + 0x0000B208))  // 32...63
#define FIQ_CONTROL ((volatile unsigned int*)(MMIO_BASE + 0x0000B20C))
#define ENABLE_IRQS_1                     \
    ((volatile unsigned int*)(MMIO_BASE + \
                              0x0000B210))  // Writing a 1 to  a bit will set
                                            // the corresponding IRQ enable bit.
                                            // (0-31)
#define ENABLE_IRQS_2 \
    ((volatile unsigned int*)(MMIO_BASE + 0x0000B214))  // (32-63)
#define ENABLE_BASIC_IRQS ((volatile unsigned int*)(MMIO_BASE + 0x0000B218))
#define DISABLE_IRQS_1 ((volatile unsigned int*)(MMIO_BASE + 0x0000B21C))
#define DISABLE_IRQS_2 ((volatile unsigned int*)(MMIO_BASE + 0x0000B220))
#define DISABLE_BASIC_IRQS \
    ((volatile unsigned int*)(MMIO_BASE + 0x0000B224))  // AUX interrupt. 29

// QA7 page. 16
#define CORE0_IRQ_SOURCE \
    ((volatile unsigned int*)(0x40000060))  // Get the IRQ source for the core 0.
