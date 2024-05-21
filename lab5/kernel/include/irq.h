#ifndef IRQ_H
#define IRQ_H



#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int*)(0x40000060))

#define PERIPHERAL_INTERRUPT_BASE 0x3F000000
#define IRQ_BASIC_PENDING ((volatile unsigned int *)(PERIPHERAL_INTERRUPT_BASE + 0x0000B200))

#define INTERRUPT_SOURCE_TIMER (1 << 1)
#define INTERRUPT_SOURCE_CNTPNSIRQ 1<<1
#define INTERRUPT_SOURCE_GPU (1 << 8)

#define INT_BASE (MMIO_BASE+0xB000)
#define IRQ_PENDING_1 ((volatile unsigned int*)(INT_BASE+0x204))
#define IRQ_PENDING_2 ((volatile unsigned int *)(PERIPHERAL_INTERRUPT_BASE + 0x0000B208))
#define IRQ_PENDING_1_AUX_INT (1 << 29)

#define FIQ_CONTROL ((volatile unsigned int *)(PERIPHERAL_INTERRUPT_BASE + 0x0000B20C))
#define ENABLE_IRQS_1 ((volatile unsigned int *)(PERIPHERAL_INTERRUPT_BASE + 0x0000B210))
#define ENABLE_IRQS_2 ((volatile unsigned int *)(PERIPHERAL_INTERRUPT_BASE + 0x0000B214))
#define ENABLE_BASIC_IRQS ((volatile unsigned int *)(PERIPHERAL_INTERRUPT_BASE + 0x0000B218))
#define DISABLE_IRQS_1 ((volatile unsigned int *)(PERIPHERAL_INTERRUPT_BASE + 0x0000B21C))
#define DISABLE_IRQS_2 ((volatile unsigned int *)(PERIPHERAL_INTERRUPT_BASE + 0x0000B220))
#define DISABLE_BASIC_IRQS ((volatile unsigned int *)(PERIPHERAL_INTERRUPT_BASE + 0x0000B224))

// trapframe register
// saved at the top of kernel stack
// sp_el1
typedef struct trapframe
{
    unsigned long x0;
    unsigned long x1;
    unsigned long x2;
    unsigned long x3;
    unsigned long x4;
    unsigned long x5;
    unsigned long x6;
    unsigned long x7;
    unsigned long x8;
    unsigned long x9;
    unsigned long x10;
    unsigned long x11;
    unsigned long x12;
    unsigned long x13;
    unsigned long x14;
    unsigned long x15;
    unsigned long x16;
    unsigned long x17;
    unsigned long x18;
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long x29;
    unsigned long x30;
    // spsr_el1 
    // 0~3 bit 0b0000 : el0t , jump to el0 and use el0 stack
    // 6~9 bit 0b0000 : turn on every interrupt
    unsigned long spsr_el1;
    unsigned long elr_el1;
    // user sp
    unsigned long sp_el0;

} trapframe_t;

void enable_interrupt();
void disable_interrupt();
void irq_handler();
void invalid_exception_handler(unsigned long long x0);
void cpacr_el1_off();
void sync_el0_64_handler(trapframe_t *tpf);

void highp();
void lowp();
void test_preemption();

void lock();
void unlock();
#endif