#define BASE_INTERRUPT_REGISTER 0x7E00B000


#define IRQ_BASIC_PENDING         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000200))
#define IRQ_PENDING_1        ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000204))
#define IRQ_PENDING_2         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000208))
#define FIQ_CONTROL         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x0000020C))
#define ENABLE_IRQS_1         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000210))
#define ENABLE_IRQS_2         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000214))
#define ENABLE_BASIC_IRQS        ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000218))
#define DISABLE_IRQS_1         ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x0000021C))
#define DISABLE_IRQS_2        ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000220))
#define DISABLE_BASIC_IRQS        ((volatile unsigned int*)(BASE_INTERRUPT_REGISTER+0x00000224))